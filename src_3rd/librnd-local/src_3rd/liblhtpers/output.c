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


   Generate styled output without existing input for a subtree
*/

static void insert_subtree_by_rules(lht_pers_t *p, const char *ind, lht_node_t *subtree);
static void export_subtree_by_rule(lht_pers_t *p, const char *ind, lht_node_t *subtree, lhtpers_rule_t *rule);

const char *lhtpers_early_end[] = { "\x01", "\xff", NULL };

static void kill_nl(char *s, int *len)
{
	int n;
	char *i = s, *o = s;

	for(n = 0; n < *len; n++,i++) {
		if (*i != '\n') {
			*o = *i;
			o++;
		}
	}
	*len = n;
}

static void indent(lht_perstyle_t *sp, lht_perslht_loc_t loc, const char *ind)
{
	if (ind != NULL) {
		lht_persbuf_t p;
		int l1, l2;
		const char *s2;


		l2 = sp->buff[loc].used;
		s2 = sp->buff[loc].s;
		if ((l2 > 0) && (s2[0] == '*')) { /* truncate leading '*' that requested the contatenation we are doing */
			l1 = strlen(ind);
			s2++;
			l2--;
			p.alloced = l1 + l2;
			p.s = malloc(p.alloced);
			memcpy(p.s, ind, l1);
			kill_nl(p.s, &l1);
			if (l2 > 0)
				memcpy(p.s+l1, s2, l2);
			p.used = l1 + l2;
		}
		else if ((l2 > 1) && (s2[l2-2] == '*')) { /* truncate trailing '*' that requested the contatenation we are doing */
/*			printf("TRAIL\n");*/
			l1 = strlen(ind);
			l2 -= 2;
			p.alloced = l1 + l2 + 1;
			p.s = malloc(p.alloced);
			memcpy(p.s, s2, l2);
			if (l2 > 0) {
				memcpy(p.s+l2, ind, l1);
				kill_nl(p.s+l2, &l1);
			}
			p.used = l1 + l2;
		}
		else
			return;
		if (sp->buff[loc].s != NULL)
			free(sp->buff[loc].s);
		memcpy(&sp->buff[loc], &p, sizeof(p));
	}
	else {
		/* indent using the first char of the prefix or space */
		if (sp->buff[loc].used > 0)
			pb_append(&sp->buff[loc], sp->buff[LHT_LOC_NAME_PRE].s[0]);
		else
			pb_append(&sp->buff[loc], ' ');
	}
}

/**** export with known style ****/
static void export_open(FILE *outf, lht_perstyle_t *sp, lht_node_type_t nt, const char *name, lht_node_t *nd)
{
	int etype = out_open_chk_etype(sp, nt, nd->parent == NULL ? LHT_INVALID_TYPE : nd->parent->type);
	const char *tmp;
	int name_braced = sp->name_braced;

	if ((!name_braced) && (lht_need_brace(nt, name, 1)))
		name_braced = 1;

	fprintf(outf, "%s", pb_gets(&sp->buff[LHT_LOC_NAME_PRE]));
	if (name_braced)
		fprintf(outf, "{");
	if (etype)
		fprintf(outf, "%s:", lht_type_id(nt));
	quote_fprint_name(outf, name);
	if (name_braced)
		fprintf(outf, "}");
	fprintf(outf, "%s", pb_gets(&sp->buff[LHT_LOC_NAME_POST]));
	if (sp->has_eq)
		fprintf(outf, "=");

	tmp = pb_gets(&sp->buff[LHT_LOC_VAL_PRE]);
	fprintf(outf, "%s", tmp);
	if (out_open_chk_brace(sp, nt, tmp))
		fprintf(outf, " {\n");
}

static void export_close(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *subtree)
{
	const char *name_pre = pb_gets(&sp->buff[LHT_LOC_NAME_PRE]);
	const char *term = pb_gets(&sp->buff[LHT_LOC_TERM]);

	/* inherited indentation shouldn't have a \n - let the user add it in TERM if it's needed */
	/* assume inherited indentation \n's would be leading */
	while(*name_pre == '\n') name_pre++;

	/* else do the more expensive kill_nl */
	if (strchr(name_pre, '\n') != NULL) {
		int len = strlen(name_pre);
		char *tmp = malloc(len+1);
		memcpy(tmp, name_pre, len);
		kill_nl(tmp, &len);
		tmp[len] = '\0';
		fprintf(p->outf, "%s", tmp);
		free(tmp);
	}
	else
		fprintf(p->outf, "%s", name_pre);
	if ((strchr(name_pre, '}') == NULL) && (strchr(term, '}') == NULL))
		fprintf(p->outf, "}");
	fprintf(p->outf, "%s", term);
}

static void export_list_with_style(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *subtree)
{
	const char *ind = pb_gets(&sp->buff[LHT_LOC_NAME_PRE]);
	lht_node_t *n;

	export_open(p->outf, sp, subtree->type, subtree->name, subtree);
	for(n = subtree->data.list.first; n != NULL; n = n->next)
		insert_subtree_by_rules(p, ind, n);
	export_close(p, sp, subtree);
}

static void export_hash_with_style(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *subtree)
{
	const char *ind = pb_gets(&sp->buff[LHT_LOC_NAME_PRE]);
	lht_node_t *n;
	lht_dom_iterator_t it;

	export_open(p->outf, sp, subtree->type, subtree->name, subtree);
	for(n = lht_dom_first(&it, subtree); n != NULL; n = lht_dom_next(&it))
		insert_subtree_by_rules(p, ind, n);
	export_close(p, sp, subtree);
}

static void export_text_with_style(lht_pers_t *p, lht_perstyle_t *sp_, lht_node_t *subtree)
{
	char *c, *postv = pb_gets(&sp_->buff[LHT_LOC_VAL_POST]);
	lht_perstyle_t sp;

	memcpy(&sp, sp_, sizeof(sp));
	sp.type = subtree->type;

	c = strchr(postv, '{');
	if (c != NULL)
		*c = '\0';
	out_text(p->outf, &sp, subtree->name, subtree->data.text.value);
	if (c != NULL)
		*c = '{';
}

static void export_table_with_style(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *subtree)
{
	/* no idea how to do this yet */
	const char *pre = pb_gets(&sp->buff[LHT_LOC_NAME_PRE]);
	lht_dom_export(subtree, p->outf, pre);
}

/**** export by rules ****/
static int name_match(const char *name, const char *pat)
{
	const char *n, *p;
	for(n = name, p = pat;; n++,p++) {
		switch(*p) {
			case '*':   return 1;
			case '\0':  return *n == '\0';
		}
		if (*n != *p)
			return 0;
	}
}

lhtpers_rule_t *lhtpers_rule_find(lhtpers_rule_t *table, lht_node_t *subtree)
{
	for(;table->path != NULL; table++) {
		const char **p;
		lht_node_t *n;

/*printf("rmatch:\n");*/
		/* ascend and follow the rule paths */
		for(n = subtree, p = table->path; (n != NULL) && (*p != NULL); n = n->parent, p++) {
			lht_node_type_t want_type;
			const char *rp = *p;

			if ((rp[0] == '*') && (rp[1] == '\0'))
				return table; /* found a rule ending in * */
			if ((rp[0] == '^') && (rp[1] == '\0')) {
				if (n->parent == NULL)
					return table; /* found a rule ending in root anchor */
				continue;
			}

			/* assume prefixed names in *p */
			assert(rp[2] == ':');
			switch(rp[0]) {
				case 't':
					if (rp[1] == 'e')
						want_type = LHT_TEXT;
					else
						want_type = LHT_TABLE;
					break;
				case 'l': want_type = LHT_LIST; break;
				case 'h': want_type = LHT_HASH; break;
				case 's': want_type = LHT_SYMLINK; break;
				default:
					goto mismatch;
			}
/*printf(" %d==%d %s~%s\n", n->type, want_type, n->name, rp+3);*/
			if ((want_type != n->type) || (!name_match(n->name, rp+3)))
				goto mismatch;
		}
		mismatch:;
	}
	return NULL;
}

static lhtpers_rule_t *lhtpers_rule_find_any(lhtpers_rule_t **tables, lht_node_t *subtree)
{
	lhtpers_rule_t **n, *res;
	for(n = tables; *n != NULL; n++) {
		res = lhtpers_rule_find(*n, subtree);
		if (res != NULL)
			return res;
	}
	return NULL;
}

static void export_hash_ordered(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *subtree, lhtpers_rule_t *rule)
{
	lhtpers_rule_t *r;
	const char *ind = pb_gets(&sp->buff[LHT_LOC_NAME_PRE]);
	htsp_t unseen;
	lht_dom_iterator_t it;
	lht_node_t *n;
	htsp_entry_t *e;
	const char *early_end_str = NULL;

	/* build a hash of items unseen while exporting by order */
	htsp_init(&unseen, strhash, strkeyeq);
	for(n = lht_dom_first(&it, subtree); n != NULL; n = lht_dom_next(&it))
		htsp_set(&unseen, n->name, n);

	/* keep the order described in the rule */
	export_open(p->outf, sp, subtree->type, subtree->name, subtree);
	for(r = rule->hash_order; r->path != NULL; r++) {
		if (r->path == lhtpers_early_end) {
			early_end_str = r->style->buff[LHT_LOC_NAME_PRE].s;
			continue;
		}
		n = lht_dom_hash_get(subtree, (*r->path)+3);
		if (n == NULL)
			continue;

		htsp_pop(&unseen, n->name);

		if (n->type != LHT_INVALID_TYPE) {
			early_end_str = NULL;
			if (r->style == NULL)
				insert_subtree_by_rules(p, ind, n);
			else
				export_subtree_by_rule(p, ind, n, r);
		}
	}

	if (early_end_str != NULL)
		fprintf(p->outf, "%s", early_end_str);

	/* write all items that were not in the order-style-list */
	for(e = htsp_first(&unseen); e; e = htsp_next(&unseen, e))
		insert_subtree_by_rules(p, ind, e->value);
	htsp_uninit(&unseen);

/*	out_close(p->outf, (lht_perstyle_t *)rule->style, subtree->type, subtree->name);*/
	export_close(p, sp, subtree);
}

/* Default styles for the case there's no style provided */
#define PB_BEGIN     {"* ", 3, 3}
#define PB_EMPTY     {"", 1, 1}
#define PB_SEMICOLON {";", 2, 2}
#define PB_SPACE     {" ", 2, 2}
#define PB_LBRACE    {"{", 2, 2}
#define PB_LBRACENL  {"{\n", 3, 3}
#define PB_RBRACENL  {"}\n", 3, 3}
#define PB_NEWLINE   {"\n", 2, 2}

static lht_perstyle_t default_struct = {
	/* buff */        {PB_BEGIN, PB_SPACE, PB_LBRACENL, PB_EMPTY, PB_EMPTY, PB_RBRACENL},
	/* has_eq */      0,
	/* val_brace */   0,
	/* etype */       1,
	/* ename */       1,
	/* name_braced */ 0
};

static lht_perstyle_t default_text = {
	/* buff */        {PB_BEGIN, PB_SPACE, PB_SPACE, PB_EMPTY, PB_EMPTY, PB_NEWLINE},
	/* has_eq */      1,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};


static void export_subtree_by_rule(lht_pers_t *p, const char *ind, lht_node_t *subtree, lhtpers_rule_t *rule)
{
	lht_perstyle_t sp, *isp;
	int hash_order = 0;

	if (rule == NULL) {
		isp = p->events->output_default_style[subtree->type];
		if (isp == NULL) {
			if ((subtree->type == LHT_TEXT) || (subtree->type == LHT_SYMLINK))
				isp = &default_text;
			else
				isp = &default_struct;
		}
	}
	else {
		isp = (lht_perstyle_t *)rule->style;
		hash_order = (rule->hash_order != NULL);
	}

	if (!isp->know_indent) {
		isp->need_indent_name_pre = (strchr(isp->buff[LHT_LOC_NAME_PRE].s, '*') != NULL);
		isp->need_indent_val_pre  = (strchr(isp->buff[LHT_LOC_VAL_PRE].s, '*') != NULL);
		isp->need_indent_val_post = (strchr(isp->buff[LHT_LOC_VAL_POST].s, '*') != NULL);
		isp->need_indent_term     = (strchr(isp->buff[LHT_LOC_TERM].s, '*') != NULL);
		isp->need_indent = (isp->need_indent_name_pre || isp->need_indent_val_pre || isp->need_indent_val_post || isp->need_indent_term);
	}

	if ((isp->buff[LHT_LOC_NAME_PRE].used > 0) && (isp->need_indent)) {
		memset(&sp, 0, sizeof(sp));
		copy_style(&sp, isp);
		sp.type = subtree->type;
		if (isp->need_indent_name_pre) indent(&sp, LHT_LOC_NAME_PRE, ind);
		if (isp->need_indent_val_pre)  indent(&sp, LHT_LOC_VAL_PRE, ind);
		if (isp->need_indent_val_post) indent(&sp, LHT_LOC_VAL_POST, ind);
		if (isp->need_indent_term)     indent(&sp, LHT_LOC_TERM, ind);
		isp = &sp;
	}

	switch(subtree->type) {
		case LHT_HASH:
			if (hash_order)
				export_hash_ordered(p, isp, subtree, rule);
			else
				export_hash_with_style(p, isp, subtree);
			break;

		case LHT_TEXT:
		case LHT_SYMLINK: export_text_with_style(p, isp, subtree);  break;
		case LHT_LIST:    export_list_with_style(p, isp, subtree);  break;
		case LHT_TABLE:   export_table_with_style(p, isp, subtree); break;

		case LHT_INVALID_TYPE:
			break;
	}

	if (isp == &sp)
		reset_style(&sp);
}

static void insert_subtree_by_rules(lht_pers_t *p, const char *ind, lht_node_t *subtree)
{
	lhtpers_rule_t *r = lhtpers_rule_find_any(p->events->output_rules, subtree);
	export_subtree_by_rule(p, ind, subtree, r); /* output by rule */
}


/**** entry point ****/


/* Insert a new subtree; try to use sp_ if it is valid (it should be the style
   last seen for a same-typed node in this context) or else alt_sp (should be
   the style of the parent) */
static void insert_subtree(lht_pers_t *p, lht_perstyle_t *sp_, lht_perstyle_t *alt_sp, lht_node_t *subtree)
{
	lht_perstyle_t sp, *spp;

	if (!sp_->valid) { /* no style picked up in this context - improvise */
		/* use sp_ if it has indentation, else try the alternative (parent's) sp */
		int need_ind = 0;

		spp = sp_;
		if (spp->buff[LHT_LOC_NAME_PRE].used < 1) {
			spp = alt_sp;
			need_ind = 1;
		}
		memset(&sp, 0, sizeof(sp));
		copy_style(&sp, spp);
		sp.type = subtree->type;

		if (need_ind)
			indent(&sp, LHT_LOC_NAME_PRE, NULL);
		pb_append(&sp.buff[LHT_LOC_NAME_PRE], '\0');

		{
			const char *ind = pb_gets(&sp.buff[LHT_LOC_NAME_PRE]);

			insert_subtree_by_rules(p, ind, subtree);
		}

		reset_style(&sp);
		return;
	}

	memset(&sp, 0, sizeof(sp));
	copy_style(&sp, sp_);
	sp.type = subtree->type;

	pb_append(&sp.buff[LHT_LOC_NAME_PRE], '\0');

	insert_subtree_by_rules(p, pb_gets(&sp.buff[LHT_LOC_NAME_PRE]), subtree);
	reset_style(&sp);
}
