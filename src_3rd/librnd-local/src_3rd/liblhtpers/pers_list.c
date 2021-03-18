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


   Handle styled output for lists.
*/


/* called for each on-disk node whose parent is a list, before printing the node */
static pre_fin_ctrl_t pers_event_li_pre(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	/* first call on a new list after a descend (or empty in-memory list)*/
	if ((parent_st->nd == NULL) && (!parent_st->inited)) {
		parent_st->nd = indoc_parent->data.list.first;
		if (parent_st->nd != NULL)
			ilprintf(p->outf, "#FIRST '%s'\n", parent_st->nd->name);
		parent_st->just_entered = 0;
		parent_st->inited = 1;
		parent_st->closing_type = LHT_LIST;
		reset_all_styles(parent_st);
	}

	ilprintf(p->outf, "#[%d] pre at %s/%s next=%s\n", p->inn_used, indoc_parent->name, sp->name, parent_st->nd != NULL ? parent_st->nd->name : "<none>");

	/* remember the style of the last children seen */
	copy_style(&parent_st->last_style[sp->type], sp);

	/* empty in-memory list: any children on-disk is to be removed */
	if (parent_st->nd == NULL) {
		ilprintf(p->outf, "##REMOVE1 %s\n", sp->name);
		if (p->events->list_empty != NULL)
			if (p->events->list_empty(p->events->ev_ctx, sp, inn_peek(p, 1, NULL)) == LHTPERS_DISK)
				return PF_SILENT_COPY;
		return PF_INHIBIT;
	}

	/* if on-disk name doesn't match in-memory name, first look if it's because
	   the on-disk name is deleted from the list */
	if (strcmp(sp->name, parent_st->nd->name) != 0) {
		lht_node_t *n;
		int found = 0;
		for(n = parent_st->nd; n != NULL; n = n->next) {
			if (strcmp(sp->name, n->name) == 0) {
				found = 1;
				break;
			}
		}
		if (!found) {
			ilprintf(p->outf, "##REMOVE2 '%s'\n", sp->name);
			if (p->events->list_elem_removed != NULL)
				if (p->events->list_elem_removed(p->events->ev_ctx, sp, parent_st->nd) == LHTPERS_DISK)
					return PF_SILENT_COPY;
			return PF_INHIBIT;
		}
	}

	/* parent_st->nd is the next node in-memory, if it doesn't match the node
	   we are about to write back from disk: it is a new node we need to insert */
	while(strcmp(sp->name, parent_st->nd->name) != 0) {
		ilprintf(p->outf, "##INSERT1 %s (!=%s)\n", parent_st->nd->name, sp->name);
		insert_subtree(p, &parent_st->last_style[parent_st->nd->type], sp, parent_st->nd);
		parent_st->nd = parent_st->nd->next;
	}

	/* we are almost in sync: same name, but different type! */
	if ((strcmp(sp->name, parent_st->nd->name) == 0) && (sp->type != parent_st->nd->type)) {
		ilprintf(p->outf, "###type mismatch, replace\n");
		/* inhibit printing the on-disk node; after finishing with that, it's
		   normal operation: the current in-memory node will look like a new
		   node and will be printed */
		return PF_INHIBIT;
	}

	parent_st->current_node = parent_st->nd;

	if (parent_st->nd != NULL) {
		/* if the new node is a list or hash or table... */
		if (is_descendable(sp)) {
			/* ... we need to descend we'll skip to next in-memory after ascending */
			ilprintf(p->outf, "##descend1 next=%s\n", parent_st->nd->name);
			parent_st = inn_push(p, parent_st->nd, sp->type);
		}
		else {
			/* new node is a plain text, skip to next on the in-memory tree */
			parent_st->nd = parent_st->nd->next;
		}
	}

	return PF_PROCEED;
}

/* called for each on-disk node whose parent is a list, after printing the node */
static void pers_event_li_post(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	ilprintf(p->outf, "#post at %s\n", indoc_parent->name);

	if (sp->type == LHT_CLOSING) {
		/* Closed the list, need to ascend */
		parent_st = inn_pop(p);

		ilprintf(p->outf, "##ascend1 now at %s\n", ((parent_st != NULL) && (parent_st->nd != NULL)) ? parent_st->nd->name : "<unknown>");

		/* we are back one level up, try to skip to next node as the disk reader
		   does that too */
		if ((parent_st != NULL) && (parent_st->nd != NULL))
			parent_st->nd = parent_st->nd->next;
	}
}

/* called for each on-disk list right before priting the closing brace */
static void pers_event_li_close(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	ilprintf(p->outf, "#CLOSE\n");

	/* Corner case: if the on-disk list is empty, we didn't have the chance
	   to initialize our new level yet; what we need to do is simple anyway:
	   just dump the list from the in-memory doc */
	if (parent_st->just_entered) {
		lht_node_t *n;
		reset_all_styles(parent_st);
		for(n = indoc_parent->data.list.first; n != NULL; n = n->next)
			insert_subtree(p, &parent_st->last_style[n->type], sp, n);
	}
	else {
		/* go through the remaining items of the in-memory list; they are sure not on
		   the on-disk list because that list has ended so print them */
		while(parent_st->nd != NULL) {
			ilprintf(p->outf, "##INSERT2 %s ind='%s'\n", parent_st->nd->name, sp->buff[LHT_LOC_NAME_PRE].s);
			insert_subtree(p, &parent_st->last_style[parent_st->nd->type], sp, parent_st->nd);
			parent_st->nd = parent_st->nd->next;
		}
	}

	reset_all_styles(parent_st);
}

