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


   This file contains type-independent tree walk helpers.
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "liblihata/dom.h"
#include "liblihata/tree.h"
#include "dom_internal.h"

lht_err_t lht_tree_detach(lht_node_t *node)
{
	if (node->parent == NULL) {
		lht_err_t err;
		lht_doc_t *doc;

		if (node->doc == NULL) /* a floating subtree is considered detached */
			return LHTE_SUCCESS;
		if (node->doc->detach_doc) /* root of a detach-doc - already detached */
			return LHTE_SUCCESS;

		/* detaching the root of an user managed document */
		doc = node->doc;
		assert(doc->root == node);
		err = lht_dom_loc_detach(node);
		doc->root = NULL;
		return err;
	}

	switch(node->parent->type) {
		case LHT_LIST:    return lht_tree_list_detach_child(node->parent, node);
		case LHT_HASH:    return lht_tree_hash_detach_child(node->parent, node);
		case LHT_TABLE:   return lht_tree_table_detach_child(node->parent, node);
		case LHT_TEXT:
		case LHT_SYMLINK:
		case LHT_INVALID_TYPE:
			break; /* return broken doc */
	}
	return LHTE_BROKEN_DOC;
}

lht_err_t lht_tree_replace(lht_node_t *node, lht_node_t *newn)
{
	switch(node->parent->type) {
		case LHT_LIST:    return lht_tree_list_replace_child(node->parent, node, newn);
		case LHT_HASH:    return lht_tree_hash_replace_child(node->parent, node, newn);
		case LHT_TABLE:   return lht_tree_table_replace_child(node->parent, node, newn);
		case LHT_TEXT:
		case LHT_SYMLINK:
		case LHT_INVALID_TYPE:
			break; /* return broken doc */
	}
	return LHTE_BROKEN_DOC;
}



lht_err_t lht_tree_unlink(lht_node_t *node)
{
	lht_err_t err;

	if (node->doc == NULL)
		return LHTE_NOT_IN_DOC;  /* not attached to a document */

	if (node->parent == NULL)
		return LHTE_WOULD_RESULT_INVALID_DOC; /* can't leave doc without a root */

	if (node->parent == node->doc->unlinked)
		return LHTE_SUCCESS;  /* already on the unlink list */

	err = lht_tree_detach(node);
	if (err != LHTE_SUCCESS)
		return err;

	return lht_dom_list_append(node->doc->unlinked, node);
}


lht_err_t lht_tree_del(lht_node_t *node)
{
	lht_err_t err;

	/* can't delete the root - the caller should uninit the doc instead */
	if ((node->doc != NULL) && (node == node->doc->root))
		return LHTE_WOULD_RESULT_INVALID_DOC;

	err = lht_tree_detach(node);
	if (err != LHTE_SUCCESS)
		return err;

	if (node->doc != NULL)
		lht_dom_uninit(node->doc); /* free the whole detached doc */
	else
		lht_dom_node_free(node); /* floating node, there's no doc to free */
	return LHTE_SUCCESS;
}

#define merge_err_(_err_) \
	do { \
		if (err != NULL) \
			*err = _err_; \
		return 0; \
	} while(0)

/* tests whether a lht_tree_merge(dst, src) would fail. */
int lht_tree_can_merge(lht_node_t *dst, lht_node_t *src, lht_err_t *err)
{
	if (dst == src)
		merge_err_(LHTE_MERGE_CYCLIC);

	if (dst->type != src->type)
		merge_err_(LHTE_MERGE_TYPE_MISMATCH);

	if (lht_tree_is_under(dst, src))
		merge_err_(LHTE_MERGE_CYCLIC);

	switch(dst->type) {
		case LHT_INVALID_TYPE:
			merge_err_(LHTE_BROKEN_DOC);
		case LHT_TEXT:
		case LHT_LIST:
			/* always can be merged */
			break;
		case LHT_SYMLINK:
			if ((*dst->data.symlink.value == '\0') || (*src->data.symlink.value == '\0'))
				merge_err_(LHTE_MERGE_EMPTY_SYMLINK);
			if (strcmp(dst->data.symlink.value, src->data.symlink.value) != 0)
				merge_err_(LHTE_MERGE_SYMLINK_MISMATCH);
			break;
		case LHT_TABLE:
			if (dst->data.table.cols != src->data.table.cols)
				merge_err_(LHTE_MERGE_TABLECOLS_MISMATCH);
			break;
		case LHT_HASH:
			{
				lht_node_t *d, *s;
				lht_dom_iterator_t it;
				for(s = lht_dom_first(&it, src); s != NULL; s = lht_dom_next(&it)) {
					d = lht_dom_hash_get(dst, s->name);
					if (d != NULL) {
						/* hash collision */
						int ret;
						ret = lht_tree_can_merge(d, s, err);
						if (ret == 0)
							return 0;
					}
				}
			}
			break;
	}

	if (err != NULL)
		*err = LHTE_SUCCESS;

	return 1;
}


void lht_tree_merge_text(lht_node_t *dst, lht_node_t *src)
{
	char *s;
	int l1, l2;
	l1 = strlen(dst->data.text.value);
	l2 = strlen(src->data.text.value);
	s = malloc(l1+l2+1);
	memcpy(s, dst->data.text.value, l1);
	memcpy(s+l1, src->data.text.value, l2+1);
	free(dst->data.text.value);
	dst->data.text.value = s;
}

void lht_tree_merge_list(lht_node_t *dst, lht_node_t *src)
{
	lht_dom_iterator_t it;
	lht_node_t *s;

	for(s = lht_dom_first(&it, src); s != NULL; s = lht_dom_next(&it)) {
		lht_tree_detach(s); /* removing an item from the list from the iterator is a special case that works since r584 */
		lht_dom_list_append(dst, s);
	}
}

lht_err_t lht_tree_merge_table(lht_node_t *dst, lht_node_t *src)
{
	int r;

	for(r = 0; r < src->data.table.rows; r++) {
		int c, dr;

		dr = dst->data.table.rows;
		if (lht_tree_table_ins_row_(dst, dr) != 0)
			return LHTE_OUT_OF_MEM;
		dst->data.table.row_names[dr] = src->data.table.row_names[r];
		src->data.table.row_names[r] = NULL;
		for (c = 0; c < dst->data.table.cols; c++) {
			dst->data.table.r[dr][c] = src->data.table.r[r][c];
			dst->data.table.r[dr][c]->doc = dst->doc;
			dst->data.table.r[dr][c]->parent = dst;
		}
	}
	src->data.table.rows = 0;
	src->data.table.cols = 0;
	return LHTE_SUCCESS;
}

lht_err_t lht_tree_merge_hash(lht_node_t *dst, lht_node_t *src, lht_err_t recurse(lht_node_t *, lht_node_t *))
{
	lht_dom_iterator_t it;
	lht_node_t *s, *d;

	for(s = lht_dom_first(&it, src); s != NULL; s = lht_dom_next(&it)) {
		lht_tree_detach(s);
		d = lht_dom_hash_get(dst, s->name);
		if (d != NULL) {
			/* hash collision */
			int ret;
			ret = recurse(d, s);
			if (ret != LHTE_SUCCESS)
				return ret;
		}
		else
			lht_dom_hash_put(dst, s);
	}
	return LHTE_SUCCESS;
}


/* Merges two nodes assuming all checks has been done and they can be merged.
   Also assume that src is already detached and can be free'd */
static lht_err_t lht_tree_merge_(lht_node_t *dst, lht_node_t *src)
{
	lht_err_t e;

	switch(dst->type) {
		case LHT_INVALID_TYPE: return LHTE_BROKEN_DOC;
		case LHT_TEXT: lht_tree_merge_text(dst, src); break;
		case LHT_LIST: lht_tree_merge_list(dst, src); break;
		case LHT_TABLE: e = lht_tree_merge_table(dst, src); if (e != LHTE_SUCCESS) return e; break;
		case LHT_HASH:  e = lht_tree_merge_hash(dst, src, lht_tree_merge_); if (e != LHTE_SUCCESS) return e; break;
		case LHT_SYMLINK:
			/* symlink text match */
			break;
	}
	lht_dom_node_free(src);
	return LHTE_SUCCESS;
}

lht_err_t lht_tree_merge(lht_node_t *dst, lht_node_t *src)
{
	lht_err_t err;
	lht_doc_t *old_doc;
	if (!lht_tree_can_merge(dst, src, &err))
		return err;

	lht_tree_detach(src);
	old_doc = src->doc;
	err = lht_tree_merge_(dst, src);
	if (old_doc != NULL) {
		old_doc->root = NULL;
		lht_dom_uninit(old_doc);
	}
	return err;
}

lht_err_t lht_tree_merge_using(lht_node_t *dst, lht_node_t *src, lht_err_t recurse(lht_node_t *, lht_node_t *))
{
	lht_err_t err;
	lht_doc_t *old_doc;
	if (!lht_tree_can_merge(dst, src, &err))
		return err;

	lht_tree_detach(src);
	old_doc = src->doc;
	err = recurse(dst, src);
	if (old_doc != NULL) {
		old_doc->root = NULL;
		lht_dom_uninit(old_doc);
	}
	return err;
}


int lht_tree_is_under(lht_node_t *node, lht_node_t *anc)
{
	for(node = node->parent; node != NULL; node = node->parent)
		if (node == anc)
			return 1;
	return 0;
}
