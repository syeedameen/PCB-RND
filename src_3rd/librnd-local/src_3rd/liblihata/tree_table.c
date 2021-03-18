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


   This file contains advanced tree walk helpers for tables.
*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "dom.h"
#include "dom_internal.h"
#include "tree.h"

static lht_node_t *lht_tree_emptyanon_text(lht_node_t *parent)
{
	lht_node_t *result;
	assert(parent->type == LHT_TABLE);

	result = lht_dom_node_alloc(LHT_TEXT, "");
	if (result != NULL) {
		result->parent = parent;
		result->doc = parent->doc;
	}
	return result;
}

lht_err_t lht_tree_table_ins_row_(lht_node_t *table, int base_row)
{
	lht_table_t *t;
	lht_err_t err;

	assert(table->type == LHT_TABLE);
	t = &table->data.table;

	if ((base_row < 0) || (base_row > t->rows))
		return LHTE_BOUNDARY;

	if ((t->rows == 0) && (t->cols == 0)) {
		err = lht_dom_table_grow_row(table);
		if (err != LHTE_SUCCESS)
			return err;
		err = lht_dom_table_grow_col(table);
		if (err != LHTE_SUCCESS)
			return err;
	}

	if (t->rows == t->rows_alloced) {
		err = lht_dom_table_grow_row(table);
		if (err != LHTE_SUCCESS)
			return err;
		if (t->cols == 0) {
			err = lht_dom_table_grow_col(table);
			if (err != LHTE_SUCCESS)
				return err;
		}
	}

	memmove(t->row_names+base_row+1, t->row_names+base_row, (t->rows - base_row) * sizeof(char *));
	memmove(t->r+base_row+1, t->r+base_row, (t->rows - base_row) * sizeof(lht_node_t **));

	t->r[base_row] = malloc(t->cols * sizeof(lht_node_t *));
	if (t->r[base_row] == NULL)
		return LHTE_OUT_OF_MEM;

	t->rows++;

	return LHTE_SUCCESS;
}

lht_err_t lht_tree_table_ins_row(lht_node_t *table, int base_row)
{
	int ret, j;
	lht_table_t *t;

	ret = lht_tree_table_ins_row_(table, base_row);
	if (ret != LHTE_SUCCESS)
		return ret;

	t = &table->data.table;

	t->row_names[base_row] = lht_strdup("");
	for (j = 0; j < t->cols; j++)
		t->r[base_row][j] = lht_tree_emptyanon_text(table);
	return LHTE_SUCCESS;
}

lht_err_t lht_tree_table_ins_col(lht_node_t *table, int base_col)
{
	lht_table_t *t;
	int i;
	lht_err_t err;

	assert(table->type == LHT_TABLE);
	t = &table->data.table;

	if ((base_col < 0) || (base_col > t->cols))
		return LHTE_BOUNDARY;

	if (t->cols == t->cols_alloced) {
		if (t->rows == 0) {
			err = lht_dom_table_grow_row(table);
			if (err != LHTE_SUCCESS)
				return err;
			err = lht_dom_table_grow_col(table);
			if (err != LHTE_SUCCESS)
				return err;
		}
	}

/*#warning TODO: why do we have col grow implementation here? Cannot grow_col() handle this? If this one is resolved, it may be that GROW_ macros can be moved back to dom_table.c and #include "dom_internal.h" can be removed from this file.*/
	if (t->cols == t->cols_alloced) {
		t->cols_alloced += LHT_TABLE_GROW_COLS;
		for (i = 0; i < t->rows; i++) {
			t->r[i] = realloc(t->r[i], t->cols_alloced * sizeof(lht_node_t *));
			if (t->r[i] == NULL) {
				t->cols_alloced -= LHT_TABLE_GROW_COLS;
				return LHTE_OUT_OF_MEM;
			}
		}
	}
	t->cols++;

	for (i = 0; i < t->rows; i++) {
		memmove(t->r[i] + base_col + 1, t->r[i] + base_col, (t->cols - base_col) * sizeof(lht_node_t *));
		t->r[i][base_col] = lht_tree_emptyanon_text(table);
	}

	return LHTE_SUCCESS;
}


/* delete the rth row of a table (freeing up all cells of that row) */
lht_err_t lht_tree_table_del_row(lht_node_t *table, int r)
{
	lht_table_t *t;
	int j;

	assert(table->type == LHT_TABLE);
	t = &table->data.table;

	if ((r < 0) || (r >= t->rows))
		return LHTE_BOUNDARY;

	free(t->row_names[r]);
	t->row_names[r] = NULL;
	memmove(t->row_names+r, t->row_names+r+1, (t->rows - r - 1) * sizeof(char *));

	for (j = 0; j < t->cols; j++) {
		lht_dom_node_free(t->r[r][j]);
		t->r[r][j] = NULL;
	}
	free(t->r[r]);
	t->r[r] = NULL;
	memmove(t->r+r, t->r+r+1, (t->rows - r - 1) * sizeof(lht_node_t **));

	t->rows--;

	return LHTE_SUCCESS;
}

/* delete the cth col of a table (freeing up all cells of that col) */
lht_err_t lht_tree_table_del_col(lht_node_t *table, int c)
{
	lht_table_t *t;
	int i;

	assert(table->type == LHT_TABLE);
	t = &table->data.table;

	if ((c < 0) || (c >= t->cols))
		return LHTE_BOUNDARY;

	for (i = 0; i < t->rows; i++) {
		lht_dom_node_free(t->r[i][c]);
		t->r[i][c] = NULL;
		memmove(&(t->r[i][c]), &(t->r[i][c+1]), (t->cols - c - 1) * sizeof(lht_node_t *));
	}

	t->cols--;

	return LHTE_SUCCESS;
}

int lht_tree_table_find_cell(lht_node_t *table, lht_node_t *node, int *row, int *col)
{
	int r, c;
	lht_table_t *t = &(table->data.table);

	assert(table->type == LHT_TABLE);

	for(r = 0; r < t->rows; r++) {
		lht_node_t **rw = t->r[r];
		for(c = 0; c < t->cols; c++) {
			if (rw[c] == node) {
				if (row != NULL) *row = r;
				if (col != NULL) *col = c;
				return 1;
			}
		}
	}
	return 0;
}

lht_node_t *lht_tree_table_named_cell(const lht_node_t *table, const char *row_name, int row_num, const char *col_name, int col_num)
{
	int n;
	lht_node_t * const *row;
	const lht_table_t *t = &(table->data.table);

	assert(table->type == LHT_TABLE);

	if (row_name == NULL) {
		/* no name is given: go the cheap way and retrieve the row pointer by idx */
		if ((row_num < 0) || (row_num >= t->rows))
			return NULL;
		row = t->r[row_num];
	}
	else {
		/* name and num: count matching names */
		row = NULL;
		for(n = 0; n < t->rows; n++) {
			if (strcmp(t->row_names[n], row_name) == 0) {
				if (row_num == 0) {
					row = t->r[n];
					break;
				}
				row_num--;
			}
		}
		if (row == NULL)
			return NULL;
	}

	/* we have a row pointer - check for the col */
	if (col_name == NULL) {
		/* no name is given: go the cheap way and retrieve the cell pointer by idx */
		if ((col_num < 0) || (col_num >= t->cols))
			return NULL;
		return (lht_node_t *)row[col_num];
	}

	/* name and num: count matching names */
	for(n = 0; n < t->cols; n++) {
		if (strcmp(row[n]->name, col_name) == 0) {
			if (col_num == 0)
				return (lht_node_t *)row[n];
			col_num--;
		}
	}

	/* row found, name:num col not found */
	return NULL;
}

lht_node_t *lht_tree_table_replace_cell_(lht_node_t *table, int row, int col, lht_err_t *err, lht_node_t *newn, int unlink)
{
	lht_node_t *ret;

	if ((row < 0) || (row >= table->data.table.rows) || (col < 0) || (col >= table->data.table.cols)) {
		if (err != NULL)
			*err = LHTE_BOUNDARY;
		return NULL;
	}
	ret = table->data.table.r[row][col];

	if (newn == NULL) {
		table->data.table.r[row][col] = lht_tree_emptyanon_text(table);
	}
	else {
		if (newn->parent != NULL) {
			if (err != NULL)
					*err = LHTE_NOT_DETACHED;
			return NULL;
		}
		newn->parent = table;
		if (newn->doc != table->doc)
			lht_dom_loc_move(table->doc, newn);

		table->data.table.r[row][col] = newn;
	}

	ret->parent = NULL;
	if (unlink) {
		/* manual unlink - ret is safely an ex-child of the table */
		lht_dom_list_append(ret->doc->unlinked, ret);
		ret->parent = ret->doc->unlinked;
	}
	else {
		lht_dom_loc_detach(ret);
	}

	if (err != NULL)
		*err = LHTE_SUCCESS;
	return ret;
}

lht_node_t *lht_tree_table_replace_cell(lht_node_t *table, int row, int col, lht_err_t *err, lht_node_t *newn)
{
	return lht_tree_table_replace_cell_(table, row, col, err, newn, 1);
}

lht_node_t *lht_tree_table_detach_cell(lht_node_t *table, int row, int col, lht_err_t *err)
{
	return lht_tree_table_replace_cell_(table, row, col, err, NULL, 0);
}


lht_err_t lht_tree_table_replace_child(lht_node_t *table, lht_node_t *existing, lht_node_t *newn)
{
	int row, col;
	lht_err_t err;

	if (lht_tree_table_find_cell(table, existing, &row, &col) == 1) {
		lht_tree_table_replace_cell_(table, row, col, &err, newn, 1);
		return err;
	}
	return LHTE_NOT_FOUND;
}


lht_err_t lht_tree_table_detach_child(lht_node_t *table, lht_node_t *node)
{
	return lht_tree_table_replace_child(table, node, NULL);
}
