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


   This file contains the DOM parser for the list node type.
*/

#include <assert.h>
#include "dom.h"
#include "dom_internal.h"

lht_err_t lht_dom_list_append(lht_node_t *dest, lht_node_t *node)
{
	assert(dest->type == LHT_LIST);
	assert(node->parent == NULL);

	if (node->doc != dest->doc)
		lht_dom_loc_move(dest->doc, node);

	node->parent = dest;
	if (dest->data.list.first == NULL) {
		assert(dest->data.list.last == NULL);
		dest->data.list.first = node;
	}
	else {
		assert(dest->data.list.last != NULL);
		dest->data.list.last->next = node;
	}
	dest->data.list.last = node;
	node->next = NULL;
	return LHTE_SUCCESS;
}

lht_err_t lht_dom_list_insert(lht_node_t *dest, lht_node_t *node)
{
	assert(dest->type == LHT_LIST);
	assert(node->parent == NULL);

	if (node->doc != dest->doc)
		lht_dom_loc_move(dest->doc, node);

	node->parent = dest;
	if (dest->data.list.first == NULL) {
		assert(dest->data.list.last == NULL);
		dest->data.list.last = node;
		node->next = NULL;
	}
	else {
		assert(dest->data.list.last != NULL);
		node->next = dest->data.list.first;
	}
	dest->data.list.first = node;

	return LHTE_SUCCESS;
}

lht_err_t lht_dom_list_insert_after(lht_node_t *dest, lht_node_t *node)
{
	assert(dest->parent != NULL);
	assert(dest->parent->type == LHT_LIST);
	assert(node->parent == NULL);

	node->parent = dest->parent;
	if (node->doc != dest->doc)
		lht_dom_loc_move(dest->doc, node);

	if (dest->parent->data.list.last == dest) {
		assert(dest->next == NULL);
		dest->data.list.last = node;
	}
	else {
		assert(dest->next != NULL);
	}

	node->next = dest->next;
	dest->next = node;

	if (node->next == NULL)
		dest->parent->data.list.last = node;

	return LHTE_SUCCESS;
}

void lht_dom_plist(lht_dom_ptcb ptree, lht_node_t *node, FILE *outf, lht_dom_indent_t *ind_, int level)
{
	lht_node_t *n;
	for (n = node->data.list.first; n != NULL; n = n->next)
		ptree(n, outf, ind_, level);
}

void lht_dom_flist(lht_node_t *node)
{
	lht_node_t *n, *m;
	for (n = node->data.list.first; n != NULL; n = m) {
		m = n->next;
		lht_dom_node_free(n);
	}
}


lht_node_t *lht_dom_list_first(lht_dom_iterator_t *it, lht_node_t *parent)
{
	if (parent->data.list.first != NULL)
		it->i.list_next = parent->data.list.first->next;
	else
		it->i.list_next = NULL;
	return parent->data.list.first;
}

lht_node_t *lht_dom_list_next(lht_dom_iterator_t *it)
{
	lht_node_t *ret;
	ret = it->i.list_next;
	if (it->i.list_next != NULL)
		it->i.list_next = it->i.list_next->next;
	return ret;
}


int lht_dom_list_len(lht_node_t *node)
{
	int result = 0;

	assert(node->type == LHT_LIST);

	for (node = node->data.list.first; node != NULL; node = node->next)
		result++;

	return result;
}
