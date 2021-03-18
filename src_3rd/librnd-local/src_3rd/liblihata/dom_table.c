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


   This file contains the DOM parser for the table node type.
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "dom.h"
#include "dom_internal.h"
#include "tree.h"

lht_err_t lht_dom_table_post_process(lht_node_t *dest, lht_node_t *row_list)
{
	lht_table_t *t;
	assert(dest->type == LHT_TABLE);
	assert(row_list->type == LHT_LIST);
	t = &dest->data.table;

	if (t->rows >= t->rows_alloced) {
		if (t->rows == 0)
			t->cols = t->cols_alloced = lht_dom_list_len(row_list);
		t->rows_alloced += LHT_TABLE_GROW_ROWS;
		t->row_names = realloc(t->row_names, t->rows_alloced * sizeof(char *));
		t->r = realloc(t->r, t->rows_alloced * t->cols_alloced * sizeof(lht_node_t *));
	}
	t->row_names[t->rows] = row_list->name;
	row_list->name = NULL;
	if (t->cols == 0) {
		t->r = NULL; /*  malloc(0) not guaranteed to return NULL */
		if (row_list->data.list.first != NULL) /* we expect an empty list */
			abort();
	}
	else {
		int i;
		lht_node_t **row, *node, *next;
		row = t->r[t->rows] = malloc(t->cols * sizeof(lht_node_t *));
		for (i = 0, node = row_list->data.list.first; node != NULL; node = next, i++) {
			if (i >= t->cols)
				return LHTE_TABLE_COLS;
			row[i] = node;
			next = node->next;
			node->parent = dest;
			node->next = NULL;
		}
		if (i != t->cols) {
			for(; i < t->cols; i++)
				row[i] = NULL;
			return LHTE_TABLE_COLS;
		}
		/* empty the list - one node can be only at one place in the tree and
		   our list items are all in table cells now */
		row_list->data.list.first = row_list->data.list.last = NULL;
	}


	t->rows++;
	return LHTE_SUCCESS;
}


void lht_dom_ptable(lht_dom_ptcb ptree, lht_node_t *node, FILE *outf, lht_dom_indent_t *ind_, int level)
{
	int i, j;
	const char *prefix;
	lht_table_t *t = &node->data.table;

	prefix = lht_dom_indent_get(ind_, level);
	for (i = 0; i < t->rows; i++) {
		fprintf(outf, "%sRow %d:{%s}\n", prefix, i, t->row_names[i]);
		for (j = 0; j < t->cols; j++)
			ptree(t->r[i][j], outf, ind_, level+1);
	}
}

void lht_dom_ftable(lht_node_t *node)
{
	lht_node_t *n;
	lht_table_t *t = &node->data.table;
	int i, j;

	if (t->r != NULL) {
		for (i = 0; i < t->rows; i++) {
			for (j = 0; j < t->cols; j++) {
				n = t->r[i][j];
				if (n != NULL)
					lht_dom_node_free(n);
			}
			if (t->r[i] != NULL)
				free(t->r[i]);
		}
	}
/*#warning TODO: related: write an empty-table regression test*/
	if (t->row_names != NULL) {
		for (i = 0; i < t->rows; i++)
			free(t->row_names[i]);
		free(t->row_names);
	}
	if (t->r != NULL)
		free(t->r);
}

int lht_dom_duptable(lht_node_t *dst_empty, lht_node_t *src_nonempty)
{
	lht_table_t *dst = &dst_empty->data.table, *src = &src_nonempty->data.table;
	int i, j, err = 0;

	/* dup/alloc administrative fields */
	dst->r = malloc(sizeof(lht_node_t **) * src->rows);
	if (dst->r == 0)
		return 1;
	dst->rows_alloced = dst->rows = src->rows;
	dst->cols_alloced = dst->cols = src->cols;
	dst->row_names = malloc(sizeof(char *) * src->rows);
	if (dst->row_names == 0)
		return 1;

	/* dup rows */
	for (i = 0; i < src->rows; i++) {
		dst->r[i] = malloc(sizeof(lht_node_t *) * src->cols);
		if (src->row_names[i] != NULL) {
			dst->row_names[i] = lht_strdup(src->row_names[i]);
			if (dst->row_names[i] == NULL)
				err++;
		}
		else
			dst->row_names[i] = NULL;

		/* dup cols */
		for (j = 0; j < src->cols; j++) {
			dst->r[i][j] = NULL;
			if (src->r[i][j] != NULL) {
				dst->r[i][j] = lht_dom_duptree(src->r[i][j]);
				if (dst->r[i][j] == NULL)
					err++;
			}
		}
	}
	return err;
}

lht_node_t *lht_dom_table_next(lht_dom_iterator_t *it)
{
	lht_node_t *ret;

	ret = lht_dom_table_cell(it->parent, it->i.tbl.row, it->i.tbl.col);
	it->i.tbl.col++;
	if (it->i.tbl.col >= it->parent->data.table.cols) {
		it->i.tbl.col = 0;
		it->i.tbl.row++;
	}

	return ret;
}


lht_node_t *lht_dom_table_first(lht_dom_iterator_t *it, lht_node_t *parent)
{
	it->i.tbl.row = 0;
	it->i.tbl.col = 0;
	return lht_dom_table_next(it);
}


lht_err_t lht_dom_table_grow_row(lht_node_t *table)
{
	lht_table_t *t;
	lht_node_t ***temp_r = NULL;
	char **temp_row_names = NULL;

	assert(table->type == LHT_TABLE);
	t = &table->data.table;

	if (t->rows < t->rows_alloced)
		return LHTE_SUCCESS; /* growing is unnecessary */

	t->rows_alloced += LHT_TABLE_GROW_ROWS;

	temp_r = realloc(t->r, t->rows_alloced * sizeof(lht_node_t **));
	temp_row_names = realloc(t->row_names, t->rows_alloced * sizeof(char *));

	if ((temp_r == NULL) || (temp_row_names == NULL)) {
		t->rows_alloced -= LHT_TABLE_GROW_ROWS;
		return LHTE_OUT_OF_MEM;
	}

	t->r = temp_r;
	t->row_names = temp_row_names;
	return LHTE_SUCCESS;
}

lht_err_t lht_dom_table_grow_col(lht_node_t *table)
{
	lht_table_t *t;
	int i;

	assert(table->type == LHT_TABLE);
	t = &table->data.table;

	if (t->cols < t->cols_alloced)
		return LHTE_SUCCESS; /* growing is unnecessary */

	t->cols_alloced += LHT_TABLE_GROW_COLS;

	for (i = 0; i < t->rows; i++) {
		lht_node_t **new_row;
		new_row = realloc(t->r[i], t->cols_alloced * sizeof(lht_node_t *));
		if (t->r[i] == NULL) {
			t->cols_alloced -= LHT_TABLE_GROW_COLS;
			return LHTE_OUT_OF_MEM;
		}
		t->r[i] = new_row;
	}

	return LHTE_SUCCESS;
}

lht_node_t *lht_dom_table_cell(lht_node_t *table, int row, int col)
{
	assert(table->type == LHT_TABLE);

	if ((row >= table->data.table.rows) || (col >= table->data.table.cols))
		return NULL;
	if ((row < 0) || (col < 0))
		return NULL;

	return table->data.table.r[row][col];
}
