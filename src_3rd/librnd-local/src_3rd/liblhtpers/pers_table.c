/*
    lhtpers - persistent lihata formatting
    Copyright (C) 2016  Tibor 'Igor2' Palinkas

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


   Handle styled output for tables.
*/


/********** ROW **********/

void insert_trow(lht_pers_t *p, lht_perstyle_t *sp, lht_perstyle_t *alt, lht_node_t *indoc_parent, int row)
{
	lht_node_t dnd;
	int n;

	memset(&dnd, 0, sizeof(dnd));
	dnd.type = LHT_LIST;
	dnd.parent = indoc_parent;

	export_open(p->outf, sp, LHT_LIST, "", &dnd);

	for(n = 0; n < indoc_parent->data.table.cols; n++) {
		lht_node_t *op, *nd = lht_dom_table_cell(indoc_parent, row, n);
		op = nd->parent;
		nd->parent = indoc_parent;
		insert_subtree(p, sp, sp, nd);
		nd->parent = op;
	}

	export_close(p, sp, &dnd);

}

/* called for each on-disk node whose parent is a table, before printing the node */
static pre_fin_ctrl_t pers_event_trow_pre(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	inst_t *new_parent_st;

	/* first call on a new list after a descend (or empty in-memory list)*/
	if (!parent_st->inited) {
		parent_st->trow = 0;
		if (parent_st->nd != NULL)
			ilprintf(p->outf, "#FIRST '%s'\n", parent_st->nd->name);
		parent_st->just_entered = 0;
		parent_st->inited = 1;
		parent_st->closing_type = LHT_TABLE;
		reset_all_styles(parent_st);
	}

	ilprintf(p->outf, "#[%d] pre at %s/%s next_row=%ld\n", p->inn_used, indoc_parent->name, sp->name, parent_st->trow);

	/* remember the style of the last children seen */
	copy_style(&parent_st->last_style[LHT_TABLE], sp);

	/* empty in-memory list: any children on-disk is to be removed */
	if (parent_st->trow >= indoc_parent->data.table.rows) {
		ilprintf(p->outf, "##REMOVE1 %s\n", sp->name);
/*
		if (p->events->table_empty != NULL)
			if (p->events->table_empty(p->events->ev_ctx, sp, inn_peek(p, 1, NULL)) == LHTPERS_DISK)
				return PF_SILENT_COPY;
*/
		return PF_INHIBIT;
	}

	ilprintf(p->outf, "##descend1 next_row=%d\n", parent_st->trow);
	new_parent_st = inn_push(p, indoc_parent, LHT_TABLE);
	new_parent_st->table_inrow = 1;
	new_parent_st->inited = 1;
	new_parent_st->trow = parent_st->trow;
	new_parent_st->tcol = -1;

	return PF_PROCEED;
}

/* called for each on-disk node whose parent is a table, after printing the table row */
static void pers_event_trow_post(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	ilprintf(p->outf, "#post at %s\n", indoc_parent->name);
	if (sp->type == LHT_CLOSING) {
		/* Closed the table, need to ascend */
		parent_st = inn_pop(p);

		ilprintf(p->outf, "##ascend1 now at %s\n", ((parent_st != NULL) && (parent_st->nd != NULL)) ? parent_st->nd->name : "<unknown>");

		/* we are back one level up, try to skip to next node as the disk reader
		   does that too */
		if ((parent_st != NULL) && (parent_st->trow <= indoc_parent->data.table.rows))
			parent_st->trow++;
	}
}

/* called for each on-disk list right before priting the table closing brace */
static void pers_event_trow_close(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	int n;
	ilprintf(p->outf, "#CLOSE\n");

	/* Corner case: if the on-disk table is empty, we didn't have the chance
	   to initialize our new level yet; what we need to do is simple anyway:
	   just dump the list from the in-memory doc */
	if (parent_st->just_entered) {
		reset_all_styles(parent_st);
		for(n = 0; n < indoc_parent->data.table.rows; n++)
			insert_trow(p, &parent_st->last_style[LHT_TABLE], sp, indoc_parent, n);
	}
	else {
		/* go through the remaining items of the in-memory list; they are sure not on
		   the on-disk list because that list has ended so print them */
		for(n = parent_st->trow; n < indoc_parent->data.table.rows; n++) {
			ilprintf(p->outf, "##INSERT2 %d\n", n);
			insert_trow(p, &parent_st->last_style[LHT_TABLE], sp, indoc_parent, n);
		}
	}

	reset_all_styles(parent_st);
}

/********** COL **********/
/* called for each on-disk node whose parent is a table row, before printing the node */
static pre_fin_ctrl_t pers_event_tcol_pre(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	lht_node_t *nd = lht_dom_table_cell(indoc_parent, parent_st->trow, parent_st->tcol);

	/* different type: output the new subtree, omit the original */
	if (sp->type != nd->type) {
		ilprintf(p->outf, "###type mismatch, replace\n");
		insert_subtree(p, &parent_st->last_style[nd->type], sp, nd);
		parent_st->tcol++;
		return PF_INHIBIT;
	}

	/* remember the style of the last children seen */
	copy_style(&parent_st->last_style[sp->type], sp);

	/* same type, different name: fix up sp */
	if (strcmp(nd->name, sp->name) != 0) {
		free(sp->name);
		sp->name = lht_strdup(nd->name);
	}

	/* set this before descending to the cell data so cell data handlers
	   know where we are in the memory-doc tree */
	parent_st->current_node = nd;

	return PF_PROCEED;
}

/* called for each on-disk node whose parent is a table row, after printing the table col */
static void pers_event_tcol_post(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	parent_st->tcol++;
}

/* called for each on-disk list right before priting the row closing brace */
static void pers_event_tcol_close(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	int n;
	/* print all the remaining nodes */
	for(n = parent_st->tcol; n < indoc_parent->data.table.cols; n++) {
		lht_node_t *nd = lht_dom_table_cell(indoc_parent, parent_st->trow, n);
		insert_subtree(p, &parent_st->last_style[nd->type], sp, nd);
	}
	parent_st->table_inrow = 0;
}

/********** DISPATCH **********/

/* called for each on-disk node whose parent is a table, before printing the node */
static pre_fin_ctrl_t pers_event_ta_pre(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	if ((!parent_st->inited) || (!parent_st->table_inrow))
		return pers_event_trow_pre(p, sp, indoc_parent, parent_st);
	return pers_event_tcol_pre(p, sp, indoc_parent, parent_st);
}

static void pers_event_ta_post(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	inst_t *st;
	inn_peek(p, 0, &st);
	if (st->table_inrow)
		pers_event_tcol_post(p, sp, indoc_parent, st);
	else
		pers_event_trow_post(p, sp, indoc_parent, st);
}

static void pers_event_ta_close(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	if (parent_st->table_inrow)
		pers_event_tcol_close(p, sp, indoc_parent, parent_st);
	else
		pers_event_trow_close(p, sp, indoc_parent, parent_st);
}
