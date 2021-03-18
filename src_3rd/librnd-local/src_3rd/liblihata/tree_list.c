/*
    liblihata - list/hash/table format, parser lib
    Copyright (C) 2013  Gabor Horvath (HvG)
    Copyright (C) 2013  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Project URLs: http://repo.hu/projects/lihata
                  svn://repo.hu/lihata


   This file contains advanced tree walk helpers for lists.
*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "dom.h"
#include "dom_internal.h"
#include "tree.h"

lht_node_t *lht_tree_list_nth(const lht_node_t *list, int n)
{
	const lht_node_t *result = NULL;

	assert(list->type == LHT_LIST);
	if (n < 0)
		return NULL;

	for (result = list->data.list.first; result != NULL && n > 0; result = result->next)
		n--;

	return (lht_node_t *)result;
}


lht_node_t *lht_tree_list_nthname(const lht_node_t *list, int n, const char *name)
{
	const lht_node_t *result = NULL;

	assert(list->type == LHT_LIST);
	if (n < 0)
		return NULL;

	for (result = list->data.list.first; result != NULL; result = result->next) {
		if (strcmp(result->name, name) == 0) {
			n--;
			if (n < 0)
				break;
		}
	}

	return (lht_node_t *)result;
}

static void list_replace(lht_node_t *list, lht_node_t *node, lht_node_t *prev, lht_node_t *newn, int unlink)
{
	if (newn == NULL) {
		/* detach: remove from the list */
		if (node == list->data.list.first)
			list->data.list.first = node->next;
		if (node == list->data.list.last)
			list->data.list.last = prev;

		if (prev != NULL)
			prev->next = node->next;
	}
	else {
		/* replace */
		if (node == list->data.list.first)
			list->data.list.first = newn;
		if (node == list->data.list.last)
			list->data.list.last = newn;

		if (prev != NULL)
			prev->next = newn;

		newn->next = node->next;
		newn->parent = node->parent;
	}

	node->next = NULL;
	node->parent = NULL;
	if (unlink) {
		lht_dom_list_append(node->doc->unlinked, node);
		node->parent = node->doc->unlinked;
	}
	else
		lht_dom_loc_detach(node);
}

lht_node_t *lht_tree_list_detach_nth(lht_node_t *list, int n)
{
	lht_node_t *node = NULL, *prev = NULL;

	assert(list->type == LHT_LIST);
	if (n < 0)
		return NULL;

	for (node = list->data.list.first; (node != NULL) && (n > 0); prev = node, node = node->next)
		n--;

	if ((n > 0) || (node == NULL))
		return NULL;

	list_replace(list, node, prev, NULL, 0);

	return node;
}

/* delete the nth element of a list (slow); returns 0 on success */
lht_err_t lht_tree_list_del_nth(lht_node_t *list, int n)
{
	lht_node_t *node = lht_tree_list_detach_nth(list, n);

	if (node == NULL)
		return LHTE_BOUNDARY;

	/* remove the whole detached document */
	lht_dom_uninit(node->doc);
	return LHTE_SUCCESS;
}

lht_err_t lht_tree_list_replace_child_(lht_node_t *list, lht_node_t *node, lht_node_t *newn, int unlink)
{
	lht_node_t *tmp = NULL;
	lht_node_t *prev = NULL;

	assert(list->type == LHT_LIST);

	if ((newn != NULL) && (newn->parent != NULL))
		return LHTE_NOT_DETACHED;

	if ((newn != NULL) && (newn->doc != list->doc))
		lht_dom_loc_move(list->doc, newn);

	for (tmp = list->data.list.first; tmp != NULL; prev = tmp, tmp = tmp->next)
		if (tmp == node)
			break;

	if (tmp == NULL) {
		if (prev == NULL)
			return LHTE_BOUNDARY;
		else if (node == NULL)
			return LHTE_BOUNDARY;

		return LHTE_NOT_FOUND;
	}

	list_replace(list, node, prev, newn, unlink);

	return LHTE_SUCCESS;
}

lht_err_t lht_tree_list_replace_child(lht_node_t *list, lht_node_t *node, lht_node_t *newn)
{
	return lht_tree_list_replace_child_(list, node, newn, 1);
}

lht_err_t lht_tree_list_detach_child(lht_node_t *list, lht_node_t *node)
{
	return lht_tree_list_replace_child_(list, node, NULL, 0);
}

/* delete the node from a list (slow); returns 0 on success */
lht_err_t lht_tree_list_del_child(lht_node_t *list, lht_node_t *node)
{
	lht_err_t ret;

	ret = lht_tree_list_detach_child(list, node);
	if (ret == LHTE_SUCCESS) {
		/* remove the whole detached document */
		lht_dom_uninit(node->doc);
	}
	return ret;
}

/* linear search for node in a list; returns index if found, -1 if not found. */
int lht_tree_list_find_node(lht_node_t *list, lht_node_t *node)
{
	int idx = 0;
	lht_node_t *tmp;

	assert(list->type == LHT_LIST);

	if (list->type == LHT_LIST) {
		for (tmp = list->data.list.first; tmp != NULL; tmp = tmp->next) {
			if (tmp == node)
				return idx;
			idx++;
		}
	}
	return -1;
}

void lht_tree_list_clean(lht_node_t *lst)
{
	lht_node_t *n;
	while (lst->data.list.first != NULL) {
		n = lst->data.list.first;
		if (n->doc == NULL) {
			if (lst->data.list.last == n)
				lst->data.list.last = NULL;
			lst->data.list.first = n->next;
		}
		else
			lht_tree_unlink(n);
		lht_dom_node_free(n);
	}
	lst->data.list.last = NULL;
}
