/*
    liblihata - list/hash/table format, parser lib
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


   This file contains advanced tree walk helpers for hashes.
*/
#include <stdlib.h>
#include <assert.h>
#include "dom.h"
#include "dom_internal.h"
#include "tree.h"

lht_err_t lht_tree_hash_detach_child(lht_node_t *parent, lht_node_t *node)
{
	lht_node_t *old;

	if (node->parent != parent) {
/*#warning TODO: this is a new error?*/
/*#warning TODO: this is a new error?*/
	}

	old = htsp_pop(parent->data.hash.tbl, node->name);
	assert(node == old);
	node->parent = NULL;
	lht_dom_loc_detach(node);
	return LHTE_SUCCESS;
}

lht_err_t lht_tree_hash_del_child(lht_node_t *parent, lht_node_t *node)
{
	lht_err_t err;
	err = lht_tree_hash_detach_child(parent, node);
	if (err == LHTE_SUCCESS)
		lht_dom_node_free(node);
	return err;
}

int lht_tree_hash_len(lht_node_t *list)
{
	return htsp_length(list->data.hash.tbl);
}

lht_err_t lht_tree_hash_replace_child(lht_node_t *hash, lht_node_t *node, lht_node_t *newn)
{
	lht_err_t ret;

	if ((newn != NULL) && (newn->parent != NULL))
		return LHTE_NOT_DETACHED;

	ret = lht_tree_unlink(node);

	if (ret != LHTE_SUCCESS)
		return ret;
	return lht_dom_hash_put(hash, newn);
}
