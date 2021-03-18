/*
    libuundo - micro-undo, a small, callback based undo/redo list implementation
    Copyright (C) 2017  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Author: libuundo [+at+] igor2.repo.hu
    VCS:    svn://repo.hu/libuundo/trunk
    Web:    http://repo.hu/projects/libuundo
*/

#include <stdlib.h>
#include <string.h>
#include "uundo.h"

void uundo_list_init(uundo_list_t *lst)
{
	memset(lst, 0, sizeof(uundo_list_t));
}

static void uundo_item_free(uundo_list_t *lst, uundo_item_t *i)
{
	/* free memory */
	if (i->oper->item_free != NULL)
		i->oper->item_free(i->udata);
	free(i);
}


static void uundo_list_remove(uundo_list_t *lst, uundo_item_t *i)
{
	/* unlink */
	if (lst->head == i)
		lst->head = i->next;
	if (lst->tail == i)
		lst->tail = i->prev;
	if (i->prev != NULL)
		i->prev->next = i->next;
	if (i->next != NULL)
		i->next->prev = i->prev;

	uundo_item_free(lst, i);
}

static void uundo_list_clear_(uundo_list_t *lst)
{
	while(lst->head != NULL)
		uundo_list_remove(lst, lst->head);
}

void uundo_list_clear(uundo_list_t *lst)
{
	uundo_list_clear_(lst);
	uundo_list_init(lst);
}

void uundo_list_uninit(uundo_list_t *lst)
{
	uundo_list_clear(lst);
}

/* remove and free all items starting from from; does not unlink */
static size_t uundo_list_truncate_from_(uundo_list_t *lst, uundo_item_t *from)
{
	uundo_item_t *i, *next;
	size_t cnt = 0;

	if (from == NULL)
		return 0;

	for(i = from; i != NULL; i = next) {
		next = i->next;
		uundo_item_free(lst, i);
		cnt++;
	}
	return cnt;
}

void uundo_list_truncate_redo(uundo_list_t *lst)
{
	if (lst->tail == NULL)
		return;

	uundo_list_truncate_from_(lst, lst->tail->next);

	lst->num_redo = 0;
	lst->tail->next = NULL;
}

/* find the first item backward from i (including i) that has a serial number
   that's lower or equal to ser */
static uundo_item_t *uundo_find_bw(uundo_item_t *i, uundo_serial_t ser)
{
	for(; i != NULL; i = i->prev)
		if (i->serial <= ser)
			return i;

	return NULL;
}

/* find the first item forward from i (including i) that has a serial number
   that's higher than or equal to ser */
static uundo_item_t *uundo_find_fw(uundo_item_t *i, uundo_serial_t ser)
{
	for(; i != NULL; i = i->next)
		if (i->serial >= ser)
			return i;

	return NULL;
}

void uundo_list_truncate_from(uundo_list_t *lst, uundo_serial_t sfirst)
{
	uundo_item_t *last;
	size_t cnt;
	int move_tail = 1;

	/* corner case: truncating from before the first item -> kill all */
	if ((lst->head != NULL) && (lst->head->serial >= sfirst)) {
		uundo_list_clear_(lst);
		lst->num_redo = lst->num_undo = 0;
		return;
	}

	/* normal case: find our target (last remaining item) in the undo section */
	last = uundo_find_bw(lst->tail, sfirst-1);
	if ((last == NULL) || (last->next == NULL))
		return;

	/* check if the item is in the redo section */
	if ((last == lst->tail) && (last->next != NULL)) {
		last = uundo_find_fw(lst->tail, sfirst);
		if (last == NULL)
			return;
		last = last->prev;
		move_tail = 0; /* this shouldn't affect the tail */
	}

	cnt = uundo_list_truncate_from_(lst, last->next);
	if (cnt > lst->num_redo) { /* both redo and undo affected */
		cnt -= lst->num_redo;
		lst->num_redo = 0;
		lst->num_undo -= cnt;
	}
	else
		lst->num_redo -= cnt; /* only redo is affected */
	last->next = NULL;
	if (move_tail)
		lst->tail = last;
}

uundo_serial_t uundo_list_find_cookie(uundo_list_t *lst, const char *cookie)
{
	uundo_item_t *i;

	for(i = lst->tail; i != NULL; i = i->prev)
		if (i->oper->cookie == cookie)
			return i->serial;

	for(i = lst->tail->next; i != NULL; i = i->next)
		if (i->oper->cookie == cookie)
			return i->serial;

	return UUNDO_SERIAL_INVALID;
}

void *uundo_append(uundo_list_t *lst, const uundo_oper_t *oper, size_t data_len)
{
	uundo_item_t *i;

	if ((lst->tail != NULL) && (lst->tail->next != NULL) && (!lst->freeze_add))
		uundo_list_truncate_redo(lst);

	i = malloc(sizeof(uundo_item_t) - 1 + data_len);

	if (!lst->freeze_add) {
		/* append at tail */
		if (lst->tail != NULL)
			lst->tail->next = i;
		else if (lst->head != NULL) {
			uundo_item_free(lst, lst->head);
			lst->head = i;
		}
		i->prev = lst->tail;
		i->next = NULL;
		lst->tail = i;
		if (lst->head == NULL)
			lst->head = i;
		lst->num_undo++;
		lst->num_redo = 0;
	}
	else {
		/* insert in front of the frozen list */
		i->next = lst->frozen;
		i->prev = NULL;
		if (lst->frozen != NULL)
			lst->frozen->prev = i;
		lst->frozen = i;
	}

	i->serial = lst->serial;
	i->oper = oper;

	return i->udata;
}

void uundo_flush_frozen(uundo_list_t *lst)
{
	while(lst->frozen != NULL) {
		uundo_item_t *i = lst->frozen;
		lst->frozen = lst->frozen->next;
		uundo_item_free(lst, i);
	}
}

/* execute undo back till s_min (including s_min), update tail */
static int uundo_undo_above_(uundo_list_t *lst, uundo_item_t *i, uundo_serial_t s_min)
{
	for(; i != NULL; i = i->prev) {
		if (i->serial < s_min)
			break;
		if ((i->oper != NULL) && (i->oper->item_undo != NULL))
			i->oper->item_undo(i->udata);
		lst->num_undo--;
		lst->num_redo++;
	}
	lst->tail = i;
	return 0;
}

int uundo_undo(uundo_list_t *lst)
{
	if (lst->tail == NULL)
		return -1;

	return uundo_undo_above_(lst, lst->tail, lst->tail->serial);
}


int uundo_undo_above(uundo_list_t *lst, uundo_serial_t s_min)
{
	if (lst->tail == NULL)
		return -1;

	return uundo_undo_above_(lst, lst->tail, s_min);
}

/* execute redo up till s_max (excluding s_max), update tail */
static int uundo_redo_below_(uundo_list_t *lst, uundo_item_t *i, uundo_serial_t s_max)
{
	for(; i != NULL; i = i->next) {
		if (i->serial >= s_max)
			break;
		if ((i->oper != NULL) && (i->oper->item_redo != NULL))
			i->oper->item_redo(i->udata);
		lst->num_undo++;
		lst->num_redo--;
		lst->tail = i;
	}
	return 0;
}

int uundo_redo(uundo_list_t *lst)
{
	if (lst->tail == NULL) {
		if (lst->head == NULL)
			return -1;
		return uundo_redo_below_(lst, lst->head, lst->head->serial+1);
	}

	if (lst->tail->next == NULL)
		return -1;

	return uundo_redo_below_(lst, lst->tail->next, lst->tail->next->serial+1);
}

int uundo_redo_below(uundo_list_t *lst, uundo_serial_t s_till)
{
	if (lst->tail == NULL) {
		if (lst->head == NULL)
			return -1;
		return uundo_redo_below_(lst, lst->head, s_till);
	}

	if (lst->tail->next == NULL)
		return -1;

	return uundo_redo_below_(lst, lst->tail->next, s_till);
}

