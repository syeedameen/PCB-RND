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

#include <stdio.h>
#include <string.h>
#include "uundo.h"
#include "uundo_debug.h"

void uundo_dump(uundo_list_t *lst, FILE *f, const char *prefix)
{
	char buff[8192];
	uundo_item_t *i;

	if (prefix == NULL)
		prefix = "";
	if (f == NULL)
		f = stdout;

	fprintf(f, "%sUndo list: num_undo=%ld num_redo=%ld freeze=%lu/%lu serial=%ld\n", prefix, (long)lst->num_undo, (long)lst->num_redo, lst->freeze_serial, lst->freeze_add, (long)lst->serial);
	for(i = lst->head; i != NULL; i = i->next) {
		char mark = ' ';
		strcpy(buff, "<unknown>");
		if (i->oper->item_print != NULL)
			i->oper->item_print(i->udata, buff, sizeof(buff));
		if ((i == lst->head) && (i == lst->tail))
			mark = '*';
		else if (i == lst->head)
			mark = 'h';
		else if (i == lst->tail)
			mark = 't';
		fprintf(f, "%s %c [%ld] %s\n", prefix, mark, (long)i->serial, buff);
	}
}

#define error(txt) \
do { \
	strcpy(msg, txt); \
	return msg; \
} while(0)

const char *uundo_check(const uundo_list_t *lst, char *msg)
{
	uundo_item_t *i, *last = NULL;
	int seen_tail = 0;
	static char msg_buff[256];
	size_t num_undo = 0, num_redo = 0;

	if (msg == NULL)
		msg = msg_buff;

	/* empty list */
	if (lst->head == NULL) {
		if (lst->num_undo != 0)
			error("lib-error: Empty list with non-zero num_undo");

		if (lst->num_redo != 0)
			error("lib-error: Empty list with non-zero num_redo");

		if (lst->tail != NULL)
			error("lib-error: Empty list with tail set");

		return NULL;
	}

	/* check first link */
	if (lst->head->prev != NULL)
		error("lib-error: first item's ->prev is not NULL");

	/* Check serial numbers and links */
	for(i = lst->head; i != NULL; i = i->next) {
		if (i != lst->head) { /* checks from the second item */
			if (i->prev->next != i) {
				sprintf(msg, "lib-error: wrong item->prev->next after %ld", (long)i->prev->serial);
				return msg;
			}
			if (i->serial < i->prev->serial) {
				sprintf(msg, "user-error: wrong serial after %ld", (long)i->prev->serial);
				return msg;
			}
		}
		if ((!seen_tail) && (lst->tail != NULL))
			num_undo++;
		else
			num_redo++;
		if (i == lst->tail)
			seen_tail = 1;
		last = i;
	}

	/* check last link */
	if (last == NULL)
		error("lib-error: no last item while head is not NULL");

	if (last->next != NULL)
		error("lib-error: last item's ->next is not NULL");

	if ((lst->tail != NULL) && (!seen_tail))
		error("lib-error: tail not found in the list");

	if (num_undo != lst->num_undo) {
		sprintf(msg, "lib-error: number of undo items mismatch: lst=%ld count=%ld", (long)lst->num_undo, (long)num_undo);
		return msg;
	}

	if (num_redo != lst->num_redo) {
		sprintf(msg, "lib-error: number of redo items mismatch: lst=%ld count=%ld", (long)lst->num_redo, (long)num_redo);
		return msg;
	}

	return NULL;
}

