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


   Handle styled output for hashes.
*/

/* use htsp because lihata already uses that */

/* called for each on-disk node whose parent is a hash, before printing the node */
static pre_fin_ctrl_t pers_event_ha_pre(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	/* first call on a new list after a descend (or empty in-memory list)*/
	if (!parent_st->inited) {
		lht_dom_iterator_t it;
		lht_node_t *n;
		htsp_init(&parent_st->ha_unseen, strhash, strkeyeq);

		for(n = lht_dom_first(&it, indoc_parent); n != NULL; n = lht_dom_next(&it))
			htsp_set(&parent_st->ha_unseen, n->name, n);
		parent_st->closing_type = LHT_HASH;
		parent_st->inited = 1;
		reset_all_styles(parent_st);
	}

	/* remember the style of the last children seen */
	copy_style(&parent_st->last_style[sp->type], sp);

	/* find the same node in the in-memory doc */
	parent_st->current_node = lht_dom_hash_get(indoc_parent, sp->name);

	/* if the on-disk node is not present in the in-memory hash, inhibit it */
	if (sp->name != NULL) {
		if ((parent_st->current_node == NULL) || (parent_st->current_node->type == LHT_INVALID_TYPE))
			return PF_INHIBIT;
	}

	/* presents both on-disk and in-memory; keep it and mark it as seen
	   (remove from the unseen hash) */
	htsp_pop(&parent_st->ha_unseen, sp->name);

	/* if the new node is a list or hash or table... */
	if (is_descendable(sp)) {
		/* ... we need to descend we'll skip to next in-memory after ascending */
		ilprintf(p->outf, "##descend1-ha at=%s\n", sp->name);
		parent_st = inn_push(p, parent_st->current_node, sp->type);
	}

	return PF_PROCEED;
}

/* called for each on-disk node whose parent is a hash, after printing the node */
static void pers_event_ha_post(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	ilprintf(p->outf, "#post-ha at %s\n", indoc_parent->name);

	if (sp->type == LHT_CLOSING) {
		/* Closed the list, need to ascend */
		parent_st = inn_pop(p);

		ilprintf(p->outf, "##ascend1-ha now at %s\n", ((parent_st != NULL) && (parent_st->current_node != NULL)) ? parent_st->current_node->name : "<unknown>");
	}
}

/* called for each on-disk hash right before priting the closing brace */
static void pers_event_ha_close(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st)
{
	htsp_entry_t *e, *nexte;

	ilprintf(p->outf, "#hash end\n");

	if (!parent_st->inited) { /* corner case: have to fill in an empty hash */
		lht_dom_iterator_t it;
		lht_node_t *n;

		for(n = lht_dom_first(&it, indoc_parent); n != NULL; n = lht_dom_next(&it)) {
			ilprintf(p->outf, "##INSERT2 %s ind='%s'\n", n->name, sp->buff[LHT_LOC_NAME_PRE].s);
			insert_subtree(p, &parent_st->last_style[n->type], sp, n);
		}
		return;
	}

	/* insert all remaining unseen items */
	for (e = htsp_first(&parent_st->ha_unseen); e; e = nexte) {
		lht_node_t *n = e->value;
		ilprintf(p->outf, "#insert '%s'\n", n->name);
		insert_subtree(p, &parent_st->last_style[n->type], sp, n);
		nexte = htsp_next(&parent_st->ha_unseen, e);
/*		htsp_delentry(&parent_st->ha_unseen, e); - no need to; no dynamic allocation */
	}
	htsp_uninit(&parent_st->ha_unseen);
	reset_all_styles(parent_st);
}
