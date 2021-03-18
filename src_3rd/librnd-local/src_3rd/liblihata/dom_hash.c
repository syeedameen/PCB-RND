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


   This file contains the DOM parser for the hash node type.
*/
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "dom.h"
#include "dom_internal.h"
#include "hash_str.h"

lht_err_t lht_dom_hash_put(lht_node_t *hash, lht_node_t *child)
{
	assert(hash->type == LHT_HASH);
	assert(child->parent == NULL);

	if (htsp_getentry(hash->data.hash.tbl, child->name) != NULL)
		return LHTE_DUPLICATE_KEY;

	if (child->doc != hash->doc)
		lht_dom_loc_move(hash->doc, child);

	child->parent = hash;

	htsp_set(hash->data.hash.tbl, child->name, child);
	return LHTE_SUCCESS;
}

lht_node_t *lht_dom_hash_get(const lht_node_t *hash, const char *name)
{
	assert(hash->type == LHT_HASH);
	return htsp_get(hash->data.hash.tbl, (char *)name);
}

void lht_dom_hash_init(lht_node_t *hash)
{
	hash->data.hash.tbl = htsp_alloc(lht_str_keyhash, lht_str_keyeq);
}

void lht_dom_phash(lht_dom_ptcb ptree, lht_node_t *node, FILE *outf, lht_dom_indent_t *ind_, int level)
{
	htsp_entry_t *e;
	for (e = htsp_first(node->data.hash.tbl); e != NULL; e = htsp_next(node->data.hash.tbl, e))
		ptree(e->value, outf, ind_, level);
}

void lht_dom_fhash(lht_node_t *node)
{
	htsp_entry_t *e;
	for (e = htsp_first(node->data.hash.tbl); e != NULL; e = htsp_next(node->data.hash.tbl, e))
		lht_dom_node_free(e->value);
	htsp_free(node->data.hash.tbl);
}

lht_node_t *lht_dom_hash_first(lht_dom_iterator_t *it, lht_node_t *parent)
{
	it->i.hash_e = htsp_first(parent->data.hash.tbl);
	if (it->i.hash_e == NULL)
		return NULL;
	return it->i.hash_e->value;
}

lht_node_t *lht_dom_hash_next(lht_dom_iterator_t *it)
{
	it->i.hash_e = htsp_next(it->parent->data.hash.tbl, it->i.hash_e);
	if (it->i.hash_e == NULL)
		return NULL;
	return it->i.hash_e->value;
}

