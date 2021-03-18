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


   This file is the main logic for learning and applying local style.
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include "lhtpers.h"


/* enable trace */
#if 0
#	define debug_printf printf
#else
	static int debugprintf_dummy(const char *fmt, ...) { return 0; }
#	define debug_printf debugprintf_dummy
#endif

/* enable some inline debug prints */
#if 0
#	define ilprintf fprintf
#else
static int ilprintf_dummy(FILE *f, ...) { return 0; }
#	define ilprintf ilprintf_dummy
#endif

#define is_descendable(nd) ((nd->type == LHT_LIST) || (nd->type == LHT_HASH) || (nd->type == LHT_TABLE))

static void pb_append(lht_persbuf_t *pb, char c)
{
	if (pb->used >= pb->alloced) {
		pb->alloced += 16;
		pb->s = realloc(pb->s, pb->alloced);
	}
	pb->s[pb->used] = c;
	pb->used++;
}

static void pb_insert(lht_persbuf_t *pb, char c)
{
	if (pb->used >= pb->alloced) {
		pb->alloced += 16;
		pb->s = realloc(pb->s, pb->alloced);
	}
	if (pb->used > 0)
		memmove(pb->s+1, pb->s, pb->used);
	pb->s[0] = c;
	pb->used++;
}

static void pb_reset(lht_persbuf_t *pb)
{
	pb->used = 0;
}

static void pb_free(lht_persbuf_t *pb)
{
	if (pb->s != NULL)
		free(pb->s);
	pb->used = pb->alloced = 0;
	pb->s = NULL;
}

static char *pb_gets(lht_persbuf_t *pb)
{
	if (pb->used == 0)
		return "";
	if ((pb->used == 0) || (pb->s[pb->used-1] != '\0'))
		pb_append(pb, '\0');
	return pb->s;
}

char *loc_accept[LHT_LOC_max] = {
	" \t\r\n",
	" \t\r\n",
	" \t\r\n",
	NULL,
	" \t\r\n",
	"\r\n;"
};

void copy_style(lht_perstyle_t *dst, lht_perstyle_t *src)
{
	int n;

	for(n = 0; n < LHT_LOC_max; n++)
		if (dst->buff[n].s != NULL)
			free(dst->buff[n].s);

	memcpy(dst, src, sizeof(lht_perstyle_t));

	for(n = 0; n < LHT_LOC_max; n++) {
		if (src->buff[n].used > 0) {
			dst->buff[n].alloced = dst->buff[n].used = src->buff[n].used;
			dst->buff[n].s = malloc(dst->buff[n].alloced);
			memcpy(dst->buff[n].s, src->buff[n].s, src->buff[n].used);
		}
		else {
			dst->buff[n].used = dst->buff[n].alloced = 0;
			dst->buff[n].s = NULL;
		}
	}
	dst->name = lht_strdup(src->name);
	dst->text_data = lht_strdup(src->text_data);
}

typedef struct {
	/* generic section */
	unsigned int del_tree, silent_copy_tree;
	lht_node_type_t closing_type;
	lht_perstyle_t last_style[LHT_SYMLINK+1];
	unsigned just_entered:1;        /* set to 1 before descending, reset to 0 right after the descend */
	unsigned inited:1;              /* set to 0 before descending, set to 1 if the level got initialized */
	lht_node_t *current_node;       /* in-memory version of the node currently being processed */

	/* type specific section */
	lht_node_t *nd;
	long trow, tcol;
	htsp_t ha_unseen;
	unsigned table_inrow:1;
} inst_t;

typedef struct lht_pers_s {
	unsigned finished:1;
	unsigned seen_root:1;

	int error;

	lht_perstyle_t *curr;
	FILE *outf;
	const lhtpers_ev_t *events;

	lht_doc_t *ind;    /* input document we are trying to save */
	lht_node_t **inn;  /* current node in the input document */
	inst_t *inst;      /* input doc per level state */
	int inn_used, inn_alloced;
} lht_pers_t;

typedef enum {
	PF_PROCEED,
	PF_INHIBIT,      /* do not print this one node */
	PF_SILENT_COPY   /* copy the on-disk subtree without further callbacks */
} pre_fin_ctrl_t;

typedef struct {
	pre_fin_ctrl_t (*pre_fin)(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st);
	void (*post_fin)(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st);
	void (*pre_close)(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_parent, inst_t *parent_st);
} hook_t;

static hook_t hooks[LHT_SYMLINK+1];

/**** input doc node stack ****/
static inst_t *inn_push(lht_pers_t *p, lht_node_t *n, lht_node_type_t closing_type)
{
	if (p->inn_used >= p->inn_alloced) {
		p->inn_alloced += 32;
		p->inn = realloc(p->inn, sizeof(lht_node_t *) * p->inn_alloced);
		p->inst = realloc(p->inst, sizeof(inst_t) * p->inn_alloced);
	}
	p->inn[p->inn_used] = n;
	memset(p->inst+p->inn_used, 0, sizeof(inst_t));
	p->inst[p->inn_used].closing_type = closing_type;
	p->inst[p->inn_used].just_entered = 1;
	p->inn_used++;
	return &p->inst[p->inn_used-1];
}

static inst_t *inn_pop(lht_pers_t *p)
{
	if (p->inn_used > 0)
		p->inn_used--;
	if (p->inn_used > 0)
		return &p->inst[p->inn_used-1];
	else
		return NULL;
}

static lht_node_t *inn_peek(lht_pers_t *p, int back, inst_t **st)
{
	int offs = p->inn_used - back - 1;
	if ((p->inn != NULL) && (offs >= 0)) {
		if (st != NULL)
			*st = p->inst + offs;
		return p->inn[offs];
	}
	if (st != NULL)
		*st = NULL;
	return NULL;
}

/**** single-node output functions ****/

/* print a name backslash-quoting all } characters */
static void quote_fprint_name(FILE *f, const char *name)
{
	const char *next = name, *last = name;
	for(;;) {
		next = strpbrk(last, "}\\");
		if (next == NULL)
			break;
		fwrite(last, next-last, 1, f);
		if (*next == '}')
			fprintf(f, "\\}");
		else
			fprintf(f, "\\\\");
		next++;
		last = next;
	}
	fprintf(f, "%s", last);
}

static void out_text(FILE *outf, lht_perstyle_t *sp, const char *name, const char *value)
{
	int val_brace_handled = 0, noname = 0;

	if (value == NULL)
		value = "";

	if (((sp->ename) || (!sp->name_braced)) && (value != NULL) && (*value != '\0') && ((name == NULL) || (*name == '\0'))) {
		/* special case: no name given */
		if (sp->val_brace) {
			sp->name_braced = 1;
			sp->val_brace = 0;
			val_brace_handled = 1;
		}
/*printf("texthack: name:|%s|%s|%s| val:|%s|%s|%s|\n", pb_gets(&sp->buff[LHT_LOC_NAME_PRE]), name, pb_gets(&sp->buff[LHT_LOC_NAME_POST]), pb_gets(&sp->buff[LHT_LOC_VAL_PRE]), value, pb_gets(&sp->buff[LHT_LOC_VAL_POST]));*/
		name = value;
		value = "";
		sp->has_eq = 0;
		noname = 1;
	}
	else if ((sp->ename == 0) && (sp->name_braced) && (*name == '\0') && (*value != '\0')) {
		/* special case: only a single {val} */
		name = value;
		value = "";
		val_brace_handled = 1;
		sp->has_eq = 0;
		noname = 1;
	}

	if ((!val_brace_handled) && (!sp->name_braced) && (*name != '\0') && (lht_need_brace(LHT_TEXT, name, noname ? 0 : 1)))
		sp->name_braced = 1;

	if ((!val_brace_handled) && (!sp->val_brace) && (*value != '\0') && (lht_need_brace(LHT_TEXT, value, 0)))
		sp->val_brace = 1;

	fprintf(outf, "%s", pb_gets(&sp->buff[LHT_LOC_NAME_PRE]));
	if ((sp->etype) || (sp->type != LHT_TEXT))
		fprintf(outf, "%s:", lht_type_id(sp->type));
	if (sp->name_braced)
		fprintf(outf, "{");
	quote_fprint_name(outf, name);
	if (sp->name_braced)
		fprintf(outf, "}");
	fprintf(outf, "%s", pb_gets(&sp->buff[LHT_LOC_NAME_POST]));
	if (sp->has_eq)
		fprintf(outf, "=");
	fprintf(outf, "%s", pb_gets(&sp->buff[LHT_LOC_VAL_PRE]));
	if (sp->val_brace)
		fprintf(outf, "{");
	quote_fprint_name(outf, value);
	if (sp->val_brace)
		fprintf(outf, "}");
	fprintf(outf, "%s", pb_gets(&sp->buff[LHT_LOC_VAL_POST]));
	fprintf(outf, "%s", pb_gets(&sp->buff[LHT_LOC_TERM]));
}

static int out_open_chk_etype(lht_perstyle_t *sp, lht_node_type_t nt, lht_node_type_t parent_nt)
{
	if (!sp->etype) {
		/* no explicit type requested, check if we absolutely have to use one */

		if (parent_nt == LHT_TABLE) {
			if (nt != LHT_LIST)
				return 1;
		}
		else if (nt != LHT_TEXT)
			return 1;
	}
	return sp->etype;
}

static int out_open_chk_brace(lht_perstyle_t *sp, lht_node_type_t nt, const char *sep)
{
	if ((nt == LHT_TEXT) || (nt == LHT_SYMLINK))
		return 0;
	if (strchr(sep, '{') == 0)
		return 1;
	return 0;
}

static void out_open(FILE *outf, lht_perstyle_t *sp, lht_node_type_t nt, const char *name)
{
	const char *tmp1, *tmp2;

	fprintf(outf, "%s", pb_gets(&sp->buff[LHT_LOC_NAME_PRE]));
	if (sp->name_braced)
		fprintf(outf, "{");
	if (sp->etype)
		fprintf(outf, "%s:", lht_type_id(nt));
	quote_fprint_name(outf, name);
	if (sp->name_braced)
		fprintf(outf, "}");
	fprintf(outf, "%s", pb_gets(&sp->buff[LHT_LOC_NAME_POST]));
	if (sp->has_eq)
		fprintf(outf, "=");

	tmp1 = pb_gets(&sp->buff[LHT_LOC_VAL_PRE]);
	tmp2 = pb_gets(&sp->buff[LHT_LOC_VAL_POST]);
	fprintf(outf, "%s", tmp1);
	if (out_open_chk_brace(sp, nt, tmp1) && out_open_chk_brace(sp, nt, tmp2))
		fprintf(outf, " {\n");

/* These have different meaning here and precede real list/hash value: */
	fprintf(outf, "%s", tmp2);
	fprintf(outf, "%s", pb_gets(&sp->buff[LHT_LOC_TERM]));
}

static void out_close(FILE *outf, lht_perstyle_t *sp, lht_node_type_t nt, const char *name)
{
	fprintf(outf, "%s}%s", pb_gets(&sp->buff[LHT_LOC_NAME_PRE]), pb_gets(&sp->buff[LHT_LOC_VAL_POST]));
	fprintf(outf, "%s", pb_gets(&sp->buff[LHT_LOC_TERM]));
}

static void reset_style(lht_perstyle_t *sp)
{
	int n;

	if (sp->name != NULL) {
		free(sp->name);
		sp->name = NULL;
	}

	if (sp->text_data != NULL) {
		free(sp->text_data);
		sp->text_data = NULL;
	}

	for(n = 0; n < LHT_LOC_max; n++)
		pb_reset(&sp->buff[n]);
	sp->type = LHT_INVALID_TYPE;
	sp->loc = LHT_LOC_NAME_PRE;
	sp->has_eq = 0;
	sp->val_brace = 0;
	sp->etype = 0;
	sp->ename = 0;
	sp->bumped_invalid_char = 0;
	sp->seen_closing_brc = 0;
	sp->composite_open = 0;
	sp->composite_close = 0;
	sp->name_braced = 0;
	sp->valid = 0;
	sp->need_indent = 0;
	sp->know_indent = 0;
}

void reset_all_styles(inst_t *p)
{
	int n;
	for(n = 0; n < sizeof(p->last_style) /sizeof(p->last_style[0]); n++)
		reset_style(&p->last_style[n]);
}

static lht_perstyle_t *lhtpers_push(lht_perstyle_t *sp)
{
	lht_perstyle_t *n = calloc(sizeof(lht_perstyle_t), 1);
	n->parent = sp;
	return n;
}

static lht_perstyle_t *lhtpers_pop(lht_perstyle_t *sp)
{
	lht_perstyle_t *ret = sp->parent;
	reset_style(sp);
	free(sp);
	return ret;
}

#define LHT_CLOSING 0x100

static void finish(lht_pers_t *p, lht_perstyle_t *sp)
{
	int delayed_push = 0, delayed_pop = 0;
	lht_node_t *parent = NULL;
	inst_t *st;
	int inhibit = 0, silent_copy = 0;

	sp->valid = 1;
	sp->need_indent = 0;
	sp->know_indent = 1;

	parent = inn_peek(p, 0, &st);
	if (parent != NULL) {
		if ((sp->type != LHT_CLOSING) && (hooks[parent->type].pre_fin != NULL)  && (sp->parent != NULL) && (st->del_tree == 0) && (st->silent_copy_tree == 0)) {
			pre_fin_ctrl_t req = hooks[parent->type].pre_fin(p, sp, parent, st);
			if (req == PF_INHIBIT) {
				if (is_descendable(sp))
					st->del_tree = 1;
				inhibit = 1;
			}
			else if (req == PF_SILENT_COPY)  {
				if (is_descendable(sp))
					st->silent_copy_tree = 1;
				silent_copy = 1;
			}
		}
	}

	if ((st != NULL) && (st->del_tree))
		inhibit = 1;
	else if ((st != NULL) && (st->silent_copy_tree))
		silent_copy = 1;

	if ((sp->type == LHT_TEXT) || (sp->type == LHT_SYMLINK)) {
		if (silent_copy) {
			out_text(p->outf, sp, sp->name, sp->text_data);
		}
		else {
			if (!inhibit) {
				if ((hooks[sp->type].pre_fin == NULL) || (hooks[sp->type].pre_fin(p, sp, (st == NULL ? NULL : st->current_node), st) != PF_INHIBIT))
					out_text(p->outf, sp, sp->name, sp->text_data);
			}
		}
	}

	if (sp->type == LHT_CLOSING) {
		if (((st == NULL) || ((st->del_tree == 0) && (st->silent_copy_tree == 0))) && (p->inn != NULL)) {
			inst_t *st2 = NULL;
			lht_node_t *parent2;
			parent2 = inn_peek(p, 0, &st2); /* (was: THIS IS A CHEAT, assuming we still have that old itme on the stack after the pop) */
			ilprintf(p->outf, "##closing type: %d\n", st2->closing_type);
			if (hooks[st2->closing_type].pre_close != NULL)
				hooks[st2->closing_type].pre_close(p, sp, parent2, st2);
		}
		else if (st != NULL) {
			if (st->del_tree > 0) {
				st->del_tree--;
				if (st->del_tree == 1) /* reached the original level in a close -> release the deltree */
					st->del_tree = 0;
			}
			else if (st->silent_copy_tree > 0) {
				st->silent_copy_tree--;
				if (st->silent_copy_tree == 1) /* reached the original level in a close -> release the deltree */
					st->silent_copy_tree = 0;
			}
		}
		if (!inhibit)
			out_close(p->outf, sp, sp->type, sp->name);
	}

	if ((sp->type == LHT_LIST) || (sp->type == LHT_HASH) || (sp->type == LHT_TABLE)) {
		if ((st != NULL) && (st->del_tree > 0))
			st->del_tree++;
		if ((st != NULL) && (st->silent_copy_tree > 0))
			st->silent_copy_tree++;
		if (!inhibit)
			out_open(p->outf, sp, sp->type, sp->name);
		delayed_push = 1;
	}

	if ((parent != NULL) && (hooks[parent->type].post_fin != NULL) && (!inhibit)) {
		inst_t *st2 = NULL;
		lht_node_t *parent2;
		parent2 = inn_peek(p, 1, &st2);
		if (parent2 != NULL)
			hooks[parent2->type].post_fin(p, sp, parent2, st2);
	}

	reset_style(sp);

	if (delayed_push)
		p->curr = lhtpers_push(sp);

	if (delayed_pop)
		p->curr = lhtpers_pop(sp);
}

/**** multi-node (subtree) output functions ****/
#include "output.c"

/**** parsing ****/
char *evname[]={
"LHT_OPEN",
"LHT_CLOSE",
"LHT_TEXTDATA",
"LHT_COMMENT",
"LHT_EOF",
"LHT_ERROR"
};

#include "pers_list.c"
#include "pers_hash.c"
#include "pers_text.c"
#include "pers_table.c"

static hook_t hooks[LHT_SYMLINK+1] = {
	{/* LHT_INVALID_TYPE */  NULL, NULL, NULL},
	{/* LHT_TEXT */          pers_event_te_pre, NULL, NULL},
	{/* LHT_LIST */          pers_event_li_pre, pers_event_li_post, pers_event_li_close},
	{/* LHT_HASH */          pers_event_ha_pre, pers_event_ha_post, pers_event_ha_close},
	{/* LHT_TABLE */         pers_event_ta_pre, pers_event_ta_post, pers_event_ta_close},
	{/* LHT_SYMLINK */       pers_event_te_pre, NULL, NULL}
};

static void pers_event(lht_parse_t *ctx, lht_event_t ev, lht_node_type_t nt, const char *name, const char *value)
{
	lht_pers_t *p = (lht_pers_t *)ctx->user_data;
	lht_perstyle_t *sp = p->curr;


	debug_printf("event %s %d\n", evname[ev], ev);
	switch (ev) {
		case LHT_OPEN:
			sp->type = nt;
			sp->name = lht_strdup(name);
			sp->loc = LHT_LOC_VAL_POST;
			sp->composite_open = 1;

			if (p->ind == NULL)
				break;

			/* descend */
			if (p->inn == NULL) { /* root node */
				p->seen_root = 1;
				if ((p->ind->root->type != nt) || (strcmp(p->ind->root->name, name) != 0)) {
					p->error = LHTPERS_ERR_ROOT_MISMATCH;
					printf("ROOT MISMATCH\n");
				}
				else
					inn_push(p, p->ind->root, nt);
			}
			break;
		case LHT_CLOSE:
			debug_printf("type=%d\n", sp->type);
			if ((sp->type != LHT_CLOSING) && (sp->type != LHT_INVALID_TYPE)) {
				finish(p, sp);
				reset_style(sp);
				sp = p->curr;
			}
			sp->type = LHT_CLOSING;
			sp->loc = LHT_LOC_VAL_POST;
			sp->composite_close = 1;
			break;
		case LHT_TEXTDATA:
			sp->type = nt;
			sp->name = lht_strdup(name);

#if 0
			/* DISABLED: at the moment the shadow parser seems to follow this correctly */
			/* strip leading whitespace if not braced */
			if (!sp->val_brace)
				while((*value == ' ') || (*value == '\t')) {
					pb_append(&sp->buff[LHT_LOC_VAL_PRE], *value);
					value++;
				}
#endif

			sp->text_data = lht_strdup(value);

			/* strip trailing whitespace if not braced */
			if (!sp->val_brace)
			{
				char *end;
				end = sp->text_data + strlen(sp->text_data) - 1;

				while((end >= sp->text_data) && ((*end == ' ') || (*end == '\t'))) {
					pb_insert(&sp->buff[LHT_LOC_VAL_POST], *end);
					*end = '\0';
					end--;
				}
			}

			sp->loc = LHT_LOC_VAL_POST;
			sp->seen_closing_brc = 0;
			break;
		case LHT_COMMENT:
			break;
		case LHT_EOF:
			p->finished = 1;
			break;
		case LHT_ERROR:
			break;
	}

}

lht_err_t lhtpers_fsave_as(const lhtpers_ev_t *events, lht_doc_t *doc, FILE *inf, FILE *outf, const char *infn, char **errmsg)
{
	int error = 0;
	lht_pers_t pectx;
	lht_parse_t par;
	int col = 1, line = 1;
	int etype, ename;
	int last_c = 0, c = 0;

	if (infn == NULL)
		infn = "(unknown)";

	lht_parser_init(&par);
	memset(&pectx, 0, sizeof(pectx));
	pectx.curr   = calloc(sizeof(lht_perstyle_t), 1);
	pectx.outf   = outf;
	pectx.ind    = doc;
	pectx.events = events;
	par.user_data = &pectx;
	par.event = pers_event;

	if (inf == NULL) { /* there's no on-disk version, need to generate one from scratch */
		lht_perstyle_t sp;
		memset(&sp, 0, sizeof(sp));
		reset_style(&sp);
		sp.valid = 1;
		sp.need_indent = 0;
		sp.know_indent = 1;
		insert_subtree(&pectx, &sp, &sp, doc->root);
		goto err;
	}


	/* parse char by char */
	while(!(feof(inf))) {
		lht_err_t err;
		lht_pstate_t pst;

		last_c = c;
		c = fgetc(inf);

		col++;
		if (c == '\n') {
			col = 1;
			line++;
		}

/*		debug_printf("= %d '%c' in %d:%d\n", lht_parser_get_state(&par), c, line, col); */
		if ((pectx.curr->composite_close) && (c == '}'))
			finish(&pectx, pectx.curr);

		/* Temporary workaround for "missing" terminator between {} {} blocks */
		if ((c == '\r') || (c == '\n') || (c == ';'))
			pectx.curr->pending_close_term = 0;

		if (c == '}')
			pectx.curr->pending_close_term = 1;

		pst = lht_parser_get_state(&par, &etype, &ename);

		if ((c == '{') && (pst == LHT_ST_BODY)) {
		 if ((pectx.curr->pending_close_term) && (pectx.curr->type == LHT_CLOSING)) {
				pectx.curr->type = LHT_CLOSING;
				pectx.curr->pending_close_term = 1;
				pectx.curr->pending_open = 0;
				pectx.curr->loc = LHT_LOC_VAL_POST;
				finish(&pectx, pectx.curr);

				pectx.curr->pending_open = 1;
				pectx.curr->loc = LHT_LOC_VAL_POST;
				pectx.curr->name_braced = 0;
			}
		}

		err = lht_parser_char(&par, c);

		if (pectx.error) {
			error = pectx.error;
			goto err;
		}

		if (pectx.curr == NULL) {
			error = LHTPERS_ERR_POPPED_TOO_MUCH;
			goto err;
		}

		pst = lht_parser_get_state(&par, &etype, &ename);
		pectx.curr->etype |= etype;
		pectx.curr->ename |= ename;
		debug_printf("  %x '%c'\n", pst, c);


		if (pst == LHT_ST_COMMENT) {
			if (pectx.curr->buff[LHT_LOC_NAME_PRE].used > 0) {
				fprintf(outf, "%s", pb_gets(&pectx.curr->buff[LHT_LOC_NAME_PRE]));
				reset_style(pectx.curr);
			}
			fputc(c, outf);
			goto skip_normal;
		}

		if ((c == '\r') || (c == '\n'))
			pectx.curr->pending_open = 0;

		/* special rules */
		switch(pectx.curr->loc) {
			case LHT_LOC_TERM:
				if ((c == '\r') || (c == '\n') || (c == ';')) {
					pb_append(&(pectx.curr->buff[pectx.curr->loc]), c);
					goto skip_normal;
				}
				else
					finish(&pectx, pectx.curr);
				break;


			case LHT_LOC_VAL_POST:
				if ((c == '{') && ((pectx.curr->type == LHT_LIST) || (pectx.curr->type == LHT_HASH) || (pectx.curr->type == LHT_TABLE))) {
					pectx.curr->pending_open = 1;
				}

				if ((c == ' ') || (c == '\t') || ((pectx.curr->composite_open) && (c == '{'))) {
					pb_append(&(pectx.curr->buff[pectx.curr->loc]), c);
					pectx.curr->composite_open = 0;
				}
				else if ((c == '\r') || (c == '\n') || (c == ';')) {
					pectx.curr->loc = LHT_LOC_TERM;
					pb_append(&(pectx.curr->buff[pectx.curr->loc]), c);
				}
				else if (c == '}') {
					if (!pectx.curr->seen_closing_brc)
						pectx.curr->seen_closing_brc = 1;
					else
						break;
				}
				else if (pectx.curr->pending_open) {
					/* a li:, ha: or ta: open in this same line */
					finish(&pectx, pectx.curr);
					break; /* first char of a new object: normal processing */
				}
				goto skip_normal;
			case LHT_LOC_NAME_PRE:
				if (c == '{')
					pectx.curr->name_braced = 1;
				else if (c == '=') {
					pectx.curr->loc = LHT_LOC_VAL_PRE;
					pectx.curr->has_eq = 1;
					goto skip_normal;
				}
				break;
			case LHT_LOC_NAME_POST:
				if (c == '=') {
					pectx.curr->loc++;
					pectx.curr->has_eq = 1;
					goto skip_normal;
				}
				if (c == '{') {
					if (last_c != ':')
						goto brc_start;
					else
						pectx.curr->name_braced = 1;
				}
				break;
			case LHT_LOC_VAL_PRE:
				if (c == '{') {
					brc_start:;
					pectx.curr->loc = LHT_LOC_VAL_IGNORE;
					pectx.curr->val_brace = 1;
					goto skip_normal;
				}
				if ((c != ' ') && (c != '\t')) {
					pectx.curr->loc = LHT_LOC_VAL_IGNORE;
					goto skip_normal;
				}
				break;
			case LHT_LOC_VAL_IGNORE:
				goto skip_normal;
			default: break;
		}

		/* normal rules to determine if we are still at that location */
		if (strchr(loc_accept[pectx.curr->loc], c) == NULL) {
			if (!pectx.curr->bumped_invalid_char) {
				pectx.curr->loc++;
				pectx.curr->bumped_invalid_char = 1;
			}
		}
		else
			pb_append(&(pectx.curr->buff[pectx.curr->loc]), c);

		skip_normal:;

		if (err != LHTE_SUCCESS) {
			if (err != LHTE_STOP) {
				if (errmsg != NULL) {
					const char *msg;

					msg = lht_err_str(err);
					*errmsg = malloc(strlen(msg) + strlen(infn) + 128);
					sprintf(*errmsg, "%s (%s:%d.%d)\n", msg, infn, line+1, col+1);
				}
				error = err;
			}
			break; /* error or stop, do not read anymore (would get LHTE_STOP without any processing all the time) */
		}
	}

	if (pectx.curr->composite_close)
		finish(&pectx, pectx.curr);

	{
		int c;
		while((c = fgetc(inf)) != EOF)
			fputc(c, outf);
	}

	if (!pectx.seen_root)
		error = LHTPERS_ERR_ROOT_MISSING;

err:;
	free(pectx.inn);
	lht_parser_uninit(&par);
	return error;
}

lht_err_t lhtpers_save_as(const lhtpers_ev_t *events, lht_doc_t *doc, const char *infn, const char *outfn, char **errmsg)
{
	FILE *inf, *outf;
	lht_err_t res;

	outf = fopen(outfn, "wb");
	if (outf == NULL)
		return LHTE_NOT_FOUND;
	inf  = fopen(infn, "rb");

	res = lhtpers_fsave_as(events, doc, inf, outf, infn, errmsg);

	if (inf != NULL)
		fclose(inf);
	fclose(outf);

	return res;
}

