/*
    liblihata - list/hash/table format, parser lib
    Copyright (C) 2013  Gabor Horvath (HvG)
    Copyright (C) 2013, 2016  Tibor 'Igor2' Palinkas

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


   This file contains the DOM parser, uses the generic API.
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "dom.h"
#include "dom_internal.h"
#include "hash_str.h"

/* set location info of the node */
static void set_loc(lht_node_t *node, const char *file_name, int line, int col)
{
	node->file_name = file_name;
	node->line = line;
	node->col = col;
}

/* append a child node to a parent - for most types we find out if it's impossible
   before we start processing an elaborate subtree. Tree is an exception. */
static lht_err_t append(lht_node_t *parent, lht_node_t *child)
{
	if (parent == NULL)
		return LHTE_BROKEN_DOC;

	switch (parent->type) {
		case LHT_LIST:
			return lht_dom_list_append(parent, child);
		case LHT_HASH:
			return lht_dom_hash_put(parent, child);
		case LHT_TABLE:
			child->parent = parent;
			return LHTE_SUCCESS; /* we do the actual job in post-process */

/* text can't have children; if we got here, the dom parser got confused */
		case LHT_TEXT:
		case LHT_SYMLINK:
			return LHTE_INTERNAL_PARSER;

/* these types shall not show up in the tree */
		case LHT_INVALID_TYPE:
			return LHTE_INTERNAL_PARSER;

/* no default: the compiler can emit a warning for unhandled cases... */
	}

	/* ... and fall back here */
	return LHTE_INTERNAL_PARSER;
}

/* post process child after it's been closed. For table rows, this is the
   place where we have enough info to insert the row in the table. */
static lht_err_t post_process(lht_doc_t *p)
{
	lht_err_t err;
	lht_node_t *row_list;

	if (p->active->parent == NULL) {
		p->active = p->active->parent;
		return LHTE_SUCCESS;
	}

	switch (p->active->parent->type) {
		case LHT_TABLE:
			row_list = p->active;
			err = lht_dom_table_post_process(p->active->parent, row_list);
			/* the row list is not in use any more, free it */
			p->active = p->active->parent;
			lht_dom_node_free(row_list);
			return err;
		/* no post processing required, but set active back to the parent */
		case LHT_LIST:
		case LHT_HASH:
			p->active = p->active->parent;
			return LHTE_SUCCESS;

/* text can't have children; if we got here, the dom parser got confused */
		case LHT_TEXT:
		case LHT_SYMLINK:
			return LHTE_INTERNAL_PARSER;

/* these types shall not show up in the tree */
		case LHT_INVALID_TYPE:
			return LHTE_INTERNAL_PARSER;

/* no default: the compiler can emit a warning for unhandled cases... */
	}

	/* ... and fall back here */
	return LHTE_INTERNAL_PARSER;
}

#define append_root_or_child() \
	do { \
		if (p->root != NULL) {\
			lht_err_t err; \
			err = append(p->active, newnode); \
			if (err != LHTE_SUCCESS) { \
				p->perror = err; \
				p->finished = 1; \
			} \
		} \
		else { \
			p->root = newnode; \
		} \
	} while(0)

static void event(lht_parse_t *ctx, lht_event_t ev, lht_node_type_t nt, const char *name, const char *value)
{
	lht_node_t *newnode;

	lht_doc_t *p = (lht_doc_t *)ctx->user_data;

	switch (ev) {
		case LHT_OPEN:
			if ((nt == LHT_TEXT) || (nt == LHT_SYMLINK)) {
				p->perror = LHTE_BROKEN_DOC;
				p->finished = 1;
				break;
			}
			newnode = lht_dom_node_alloc(nt, name);
			newnode->doc = p;
			set_loc(newnode, p->active_file, ctx->line, ctx->col);
			append_root_or_child();
			p->active = newnode;
			break;
		case LHT_CLOSE:
			post_process(p);
			break;
		case LHT_TEXTDATA:
			if ((nt != LHT_TEXT) && (nt != LHT_SYMLINK)) {
				p->perror = LHTE_BROKEN_DOC;
				p->finished = 1;
				break;
			}
			newnode = lht_dom_node_alloc(nt, name);
			newnode->doc = p;
			set_loc(newnode, p->active_file, ctx->line, ctx->col);
			if (p->active == NULL)
				p->active = p->root;
			append_root_or_child();
			newnode->data.text.value = lht_strdup(value);
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

#undef append_root_or_child

lht_node_t *lht_dom_node_alloc(lht_node_type_t type, const char *name)
{
	lht_node_t *newnode;

	if ((type <= LHT_INVALID_TYPE) || (type > LHT_SYMLINK))
		return NULL;

	newnode = calloc(1, sizeof(lht_node_t));
	if (newnode == NULL)
		return NULL;

	if (name == NULL)
		name = "";

	newnode->type = type;
	newnode->name = lht_strdup(name);

	/* some types will require extra setup */
	switch(type) {
		case LHT_HASH:
			lht_dom_hash_init(newnode);
			break;
		default:
			/* no extra, suppress compiler warnings */
			;
	}

	return newnode;
}

lht_doc_t *lht_dom_init(void)
{
	lht_doc_t *doc = malloc(sizeof(lht_doc_t));
	doc->root = NULL;
	doc->detach_doc = 0;
	doc->unlinked = lht_dom_node_alloc(LHT_LIST, "<unlinked>");
	doc->unlinked->doc = doc;
	doc->active = NULL;
	doc->active_file = NULL;
	doc->err_line = -1;
	doc->err_col = -1;
	doc->file_names = htsp_alloc(lht_str_keyhash, lht_str_keyeq);
	doc->p = malloc(sizeof(lht_parse_t));

	lht_parser_init(doc->p);
	doc->p->event = event;
	doc->p->user_data = doc;

	doc->finished = 0;
	doc->perror = LHTE_SUCCESS;

	return doc;
}

void lht_dom_node_free(lht_node_t *node)
{
	switch (node->type) {
		case LHT_LIST:
			lht_dom_flist(node);
			break;
		case LHT_TABLE:
			lht_dom_ftable(node);
			break;
		case LHT_HASH:
			lht_dom_fhash(node);
			break;

/* text and symlink can't have children */
		case LHT_TEXT:
		case LHT_SYMLINK:
			free(node->data.text.value);
			break;

/* Invalid type means the node is allocated but its fields are not */
		case LHT_INVALID_TYPE:
			break;
	}
	free(node->name);
	free(node);
}

/* uninit/free the parser if it's active */
static void dom_parser_uninit(lht_doc_t *doc)
{
	if (doc->p != NULL) {
		doc->err_line = doc->p->line;
		doc->err_col = doc->p->col;
		lht_parser_uninit(doc->p);
		free(doc->p);
		doc->p = NULL;
	}
}

void lht_dom_uninit(lht_doc_t *doc)
{
	htsp_entry_t *e;

	dom_parser_uninit(doc);
	if (doc->root != NULL)
		lht_dom_node_free(doc->root);
	lht_dom_node_free(doc->unlinked);

	for (e = htsp_first(doc->file_names); e != NULL; e = htsp_next(doc->file_names, e))
		free(e->key);
	htsp_free(doc->file_names);

	free(doc);
}

lht_err_t lht_dom_parser_char(lht_doc_t *doc, int c)
{
	lht_err_t ret;

	if (doc->finished) {
		if (doc->perror == LHTE_SUCCESS)
			return LHTE_STOP;
		else
			return doc->perror;
	}

	ret = lht_parser_char(doc->p, c);

	if (ret != LHTE_SUCCESS) {
		doc->finished = 1;
		dom_parser_uninit(doc);
	}
	return ret;
}


void lht_dom_pnode(lht_node_t *node, FILE *outf, const char *prefix)
{
	fprintf(outf, "%s%s:{%s} ", prefix, lht_type_id(node->type), node->name);

	if (node->parent == NULL)
		fprintf(outf, "[root] ");

	if (node->doc != NULL) {
		if (node->parent == node->doc->unlinked)
			fprintf(outf, "[unlinked] ");
		if (node->doc->detach_doc)
			fprintf(outf, "[detached] ");
	}
	else
		fprintf(outf, "[nodoc] ");

	switch (node->type) {
		case LHT_TEXT:
		case LHT_SYMLINK:
			fprintf(outf, "{%s}\n", node->data.text.value);
			break;
		case LHT_LIST:
			if (node->data.list.first != NULL)
				fprintf(outf, "FIRST %s:{%s} ", lht_type_id(node->data.list.first->type), node->data.list.first->name);
			if (node->data.list.last != NULL)
				fprintf(outf, "LAST %s:{%s}", lht_type_id(node->data.list.last->type), node->data.list.last->name);
			fprintf(outf, "\n");
			break;
		case LHT_TABLE:
			fprintf(outf, "cols: %d, rows: %d\n", node->data.table.cols, node->data.table.rows);
			break;
		case LHT_HASH:
			fprintf(outf, "entries: %d\n", node->data.hash.tbl->used);
			break;
		case LHT_INVALID_TYPE:
			fprintf(outf, "ERROR: invalid type.\n");
			break;
		default:
			fprintf(outf, "ERROR: unknown type.\n");
			break;
	}
}


struct lht_dom_indent_s {
	int prefix_len, alloced;
	const char *prefix;
	char *s;
};
#define INDENT_GROWTH 64

static void indent_alloc(lht_dom_indent_t *ind, int levels)
{
	ind->alloced = levels + ind->prefix_len;
	ind->s = malloc(levels + ind->prefix_len + 1);
	memset(ind->s, ' ', levels);
	memcpy(ind->s + levels, ind->prefix, ind->prefix_len + 1);
}

static void indent_free(lht_dom_indent_t *ind)
{
	free(ind->s);
	ind->alloced = 0;
}

static void indent_setup(lht_dom_indent_t *ind, const char *prefix)
{
	ind->prefix_len = strlen(prefix);
	ind->alloced = 0;
	ind->prefix = prefix;
	indent_alloc(ind, INDENT_GROWTH);
}

const char *lht_dom_indent_get(lht_dom_indent_t *ind, int level)
{
	return (ind)->s + (ind)->alloced - (ind)->prefix_len - (level);
}

static void ptree(lht_node_t *node, FILE *outf, lht_dom_indent_t *ind, int level)
{
	const char *prefix;

	/* set up prefix according to level */
	if (level + ind->prefix_len > ind->alloced) {
		indent_free(ind);
		indent_alloc(ind, level + INDENT_GROWTH);
	}
	prefix = lht_dom_indent_get(ind, level);

	/* print the current node */
	lht_dom_pnode(node, outf, prefix);

	/* print children */
	switch (node->type) {
		case LHT_TEXT:
		case LHT_SYMLINK:
			break; /* no children nodes */
		case LHT_LIST:
			lht_dom_plist(ptree, node, outf, ind, level+1);
			break;
		case LHT_TABLE:
			lht_dom_ptable(ptree, node, outf, ind, level+1);
			break;
		case LHT_HASH:
			lht_dom_phash(ptree, node, outf, ind, level+1);
			break;
		case LHT_INVALID_TYPE:
			fprintf(outf, "ERROR: invalid type.\n");
			break;
		default:
			fprintf(outf, "ERROR: unknown type.\n");
			break;
	}
}

void lht_dom_ptree(lht_node_t *node, FILE *outf, const char *prefix)
{
	lht_dom_indent_t ind;
	indent_setup(&ind, prefix);
	ptree(node, outf, &ind, 0);
	indent_free(&ind);
}

int lht_need_brace(lht_node_type_t type, const char *txt, int is_name)
{
	const char *s;
	int trailing_ws = 0;

	if ((txt == NULL) || (*txt == '\0')) {
		if ((is_name) && (type != LHT_TEXT))
			return 0; /* empty name without {} is ok except for text where te: is usually implicit */
		return 1;
	}

	/* leading whitespace */
	if ((*txt == ' ') || (*txt == '\t'))
		return 1;

	/* control chars in text */
	for(s = txt; *s != '\0'; s++) {
		if (is_name && (*s == '/'))
			return 1;
		if ((*s == '\\') || (*s == ':') || (*s == ';') || (*s == '\n') || (*s == '\r') || (*s == '{') || (*s == '}') || (*s == '=') || (*s == '#'))
			return 1;
		if ((*s == ' ') || (*s == '\t')) {
			if (is_name)
				return 1;
			trailing_ws = 1;
		}
		else
			trailing_ws = 0;
	}

	/* trailing whitespace */
	if (trailing_ws)
		return 1;

	/* nothing to protect */
	return 0;
}

static void lht_dom_export_(lht_node_t *node, FILE *outf, lht_dom_indent_t *ind, int level, int intable, lht_dom_export_style_t exp_style)
{
	const char *prefix, *nb1, *nb2, *vb1, *vb2, *val, *eq, *eq_normal = "= ";
	lht_node_t *subnode;
	lht_dom_iterator_t it;
	int row, col;
	lht_dom_export_style_t style;

	if (level + ind->prefix_len > ind->alloced) {
		indent_free(ind);
		indent_alloc(ind, level + INDENT_GROWTH);
	}
	prefix = lht_dom_indent_get(ind, level);

	style = exp_style;
	if ((style & LHT_STY_BRACE_NAME) || lht_need_brace(node->type, node->name, 1)) {
		nb1 = "{";
		nb2 = "}";
	}
	else
		nb1 = nb2 = "";
	eq = "";

	vb1 = vb2 = "";

	switch (node->type) {
		case LHT_TEXT:
			if (style & LHT_STY_EQ_TE)
				eq = eq_normal;
			val = (node->data.text.value == NULL) ? "" : node->data.text.value;
			if ((style & LHT_STY_BRACE_TE) || lht_need_brace(node->type, val, 0)) {
				vb1 = "{";
				vb2 = "}";
			}
			if (!intable)
				fprintf(outf, "%s", prefix);
			if (strlen(node->name) > 0)
				fprintf(outf, "%s%s%s %s%s%s%s", nb1, node->name, nb2,  eq,  vb1, val, vb2);
			else
				fprintf(outf, "%s%s%s", vb1, val, vb2);
			if (!intable)
				fprintf(outf, "\n");
			break;
		case LHT_SYMLINK:
			if (style & LHT_STY_EQ_SY)
				eq = eq_normal;
			val = (node->data.symlink.value == NULL) ? "" : node->data.symlink.value;
			if ((style & LHT_STY_BRACE_SY) || lht_need_brace(node->type, val, 0)) {
				vb1 = "{";
				vb2 = "}";
			}
			fprintf(outf, "%s%s%s:%s%s %s%s%s%s", prefix, nb1, lht_type_id(node->type), node->name, nb2,  eq,  vb1, val, vb2);
			if (!intable)
				fprintf(outf, "\n");
			break;
		case LHT_LIST:
			if (style & LHT_STY_EQ_LI)
				eq = eq_normal;
			else
		case LHT_HASH:
			if (style & LHT_STY_EQ_HA)
				eq = eq_normal;
			if (!intable)
				fprintf(outf, "%s%s%s:%s%s %s{\n", prefix, nb1, lht_type_id(node->type), node->name, nb2, eq);
			else
				fprintf(outf, "%s%s:%s%s %s{ ", nb1, lht_type_id(node->type), node->name, nb2, eq);
			for (subnode = lht_dom_first(&it, node); subnode != NULL; subnode = lht_dom_next(&it)) {
				lht_dom_export_(subnode, outf, ind, level + 1, intable, exp_style);
				if (intable)
					fprintf(outf, "; ");
			}
			if (!intable)
				fprintf(outf, "%s", prefix);
			fprintf(outf, "}");
			if (!intable)
				fprintf(outf, "\n");
			break;
		case LHT_TABLE:
			if (style & LHT_STY_EQ_TA)
				eq = eq_normal;
			fprintf(outf, "%s%s%s:%s%s %s{\n", prefix, nb1, lht_type_id(node->type), node->name, nb2, eq);
			row = col = 0;
			if (style & LHT_STY_EQ_TA_ROW)
				eq = eq_normal;
			for (subnode = lht_dom_first(&it, node); subnode != NULL; subnode = lht_dom_next(&it)) {
				if (col == 0) {
					if (strlen(node->data.table.row_names[row]) > 0) {
						if (lht_need_brace(node->type, node->data.table.row_names[row], 1))
							fprintf(outf, "%s{%s} %s{ ", lht_dom_indent_get(ind, level + 1), node->data.table.row_names[row], eq);
						else
							fprintf(outf, "%s%s %s{ ", lht_dom_indent_get(ind, level + 1), node->data.table.row_names[row], eq);
					}
					else
						fprintf(outf, "%s{ ", lht_dom_indent_get(ind, level + 1));
				}
				lht_dom_export_(subnode, outf, ind, 0, 1, exp_style);
				if (col == node->data.table.cols - 1) {
					fprintf(outf, " }\n");
					col = -1;
					row++;
				}
				else {
					fprintf(outf, "; ");
				}
				col++;
			}
			fprintf(outf, "%s}\n", prefix);
			break;
		case LHT_INVALID_TYPE:
			fprintf(outf, "ERROR: invalid type.\n");
			break;
		default:
			fprintf(outf, "ERROR: unknown type.\n");
			break;
	}
}

lht_err_t lht_dom_export_style(lht_node_t *node, FILE *outf, const char *prefix, lht_dom_export_style_t exp_style)
{
	lht_dom_indent_t ind;
	indent_setup(&ind, prefix);
	lht_dom_export_(node, outf, &ind, 0, 0, exp_style);
	indent_free(&ind);
	return LHTE_SUCCESS;
}

lht_err_t lht_dom_export(lht_node_t *node, FILE *outf, const char *prefix)
{
	return lht_dom_export_style(node, outf, prefix, LHT_STY_EQ_TE | LHT_STY_EQ_SY);
}

lht_node_t *lht_dom_first(lht_dom_iterator_t *it, lht_node_t *parent)
{
	it->parent = parent;
	switch(parent->type) {
		case LHT_LIST:    return lht_dom_list_first(it, parent);
		case LHT_HASH:    return lht_dom_hash_first(it, parent);
		case LHT_TABLE:   return lht_dom_table_first(it, parent);
		case LHT_TEXT:    return NULL;
		case LHT_SYMLINK: return NULL;
		case LHT_INVALID_TYPE: return NULL;
	}
	return NULL;
}

/* returns the next child using an iterator set up by lht_dom_first() */
lht_node_t *lht_dom_next(lht_dom_iterator_t *it)
{
	switch(it->parent->type) {
		case LHT_LIST:    return lht_dom_list_next(it);
		case LHT_HASH:    return lht_dom_hash_next(it);
		case LHT_TABLE:   return lht_dom_table_next(it);
		case LHT_TEXT:    return NULL;
		case LHT_SYMLINK: return NULL;
		case LHT_INVALID_TYPE: return NULL;
	}
	return NULL;
}


/* === location manipulation === */
int lht_dom_loc_newfile(lht_doc_t *doc, const char *name)
{
	htsp_entry_t *e;
	doc->p->line = 0;
	doc->p->col  = 0;
	e = htsp_getentry(doc->file_names, (char *)name);
	if (e == NULL) {
		const char *active_name;
		active_name = lht_strdup(name);
		htsp_set(doc->file_names, (char *)active_name, doc);
		doc->active_file = active_name;
		return 0;
	}
	doc->active_file = e->key;
	return 1;
}

void lht_dom_loc_active(lht_doc_t *doc, const char **fn, int *line, int *col)
{
	if (fn != NULL)
		*fn = doc->active_file;
	if (line != NULL)
		*line = doc->p == NULL ? doc->err_line : doc->p->line;
	if (col != NULL)
		*col = doc->p == NULL ? doc->err_col : doc->p->col;
}

static int lht_dom_loc_move_(lht_doc_t *doc, lht_node_t *node)
{
	const char *local_name;
	int new_fn = 0;
	lht_dom_iterator_t it;

	if (doc != NULL) {
		if (node->file_name != NULL) {
			htsp_entry_t *ent;
			ent = htsp_getentry(doc->file_names, (char *)node->file_name);
			if (ent == NULL) {
				local_name = lht_strdup(node->file_name);
				htsp_set(doc->file_names, (char *)local_name, doc);
				new_fn = 1;
			}
			else
				local_name = ent->key;
			node->file_name = local_name;
		}
	}
	node->doc = doc;

	/* depth-first move all children */
	for(node = lht_dom_first(&it, node); node != NULL; node = lht_dom_next(&it))
		lht_dom_loc_move_(doc, node);

	return new_fn;
}

int lht_dom_loc_move(lht_doc_t *doc, lht_node_t *node)
{
	lht_doc_t *old_doc;
	int new_fn, auto_free;

	old_doc = node->doc;
	if (old_doc != NULL)
		auto_free = ((old_doc->root == node) && (old_doc->detach_doc));
	else
		auto_free = 0;

	new_fn = lht_dom_loc_move_(doc, node);

	if (auto_free) {
		old_doc->root = NULL;
		lht_dom_uninit(old_doc);
	}
	return new_fn;
}


lht_err_t lht_dom_loc_detach(lht_node_t *node)
{
	lht_doc_t *doc;

	if ((node->doc != NULL) && (node->parent != NULL))
		return LHTE_NOT_DETACHED;

	doc = lht_dom_init();
	doc->detach_doc = 1;
	lht_dom_loc_move(doc, node);
	doc->root = node;
	return LHTE_SUCCESS;
}

/* === helpers === */
lht_doc_t *lht_dom_load_stream(FILE *f, const char *fn, char **errmsg)
{
	lht_doc_t *doc;
	int error = 0;


	/* set up for parsing */
	doc = lht_dom_init();
	lht_dom_loc_newfile(doc, fn);

	/* parse char by char */
	while(!(feof(f))) {
		lht_err_t err;
		int c = fgetc(f);
		err = lht_dom_parser_char(doc, c);
		if (err != LHTE_SUCCESS) {
			if (err != LHTE_STOP) {
				if (errmsg != NULL) {
					const char *fn;
					int line, col;
					const char *msg;

					lht_dom_loc_active(doc, &fn, &line, &col);
					msg = lht_err_str(err);
					*errmsg = malloc(strlen(msg) + strlen(fn) + 128);
					sprintf(*errmsg, "%s (%s:%d.%d)\n", msg, fn, line+1, col+1);
				}
				error = 1;
			}
			break; /* error or stop, do not read anymore (would get LHTE_STOP without any processing all the time) */
		}
	}


	if (error) {
		lht_dom_uninit(doc);
		return NULL;
	}

	return doc;
}


lht_doc_t *lht_dom_load(const char *fn, char **errmsg)
{
	FILE *f;
	lht_doc_t *doc;

	/* open the file or exit */
	f = fopen(fn, "r");
	if (f == NULL) {
		if (errmsg != NULL) {
			*errmsg = malloc(strlen(fn) + 128);
			sprintf(*errmsg, "can't open '%s' for read\n", fn);
		}
		return NULL;
	}

	doc = lht_dom_load_stream(f, fn, errmsg);

	fclose(f);
	return doc;
}

lht_doc_t *lht_dom_load_string(const char *str, const char *fn, char **errmsg)
{
	lht_doc_t *doc;
	int error = 0;

	doc = lht_dom_init();
	lht_dom_loc_newfile(doc, fn);

	while(*str != '\0') {
		lht_err_t err;
		int c = *str++;
		err = lht_dom_parser_char(doc, c);
		if (err != LHTE_SUCCESS) {
			if (err != LHTE_STOP) {
				if (errmsg != NULL) {
					const char *fn;
					int line, col;
					const char *msg;

					lht_dom_loc_active(doc, &fn, &line, &col);
					msg = lht_err_str(err);
					*errmsg = malloc(strlen(msg) + strlen(fn) + 128);
					sprintf(*errmsg, "%s (%s:%d.%d)\n", msg, fn, line+1, col+1);
				}
				error = 1;
				break;
			}
			break; /* error or stop, do not read anymore (would get LHTE_STOP without any processing all the time) */
		}
	}

	if (error) {
		lht_dom_uninit(doc);
		doc = NULL;
	}

	return doc;
}


lht_node_t *lht_dom_duptree(lht_node_t *src)
{
	lht_dom_iterator_t it;
	lht_node_t *dst, *n;

	dst = lht_dom_node_alloc(src->type, src->name);

	switch(src->type) {
		case LHT_INVALID_TYPE:
			return NULL;
		case LHT_TEXT:
		case LHT_SYMLINK:
			dst->data.text.value = lht_strdup(src->data.text.value);
			break; /* no children, don't copy them */
		case LHT_LIST:
			for(n = lht_dom_first(&it, src); n != NULL; n = lht_dom_next(&it))
				lht_dom_list_append(dst, lht_dom_duptree(n));
			break;
		case LHT_HASH:
			for(n = lht_dom_first(&it, src); n != NULL; n = lht_dom_next(&it))
				lht_dom_hash_put(dst, lht_dom_duptree(n));
			break;
		case LHT_TABLE:
			lht_dom_duptable(dst, src);
			break;
	}

	return dst;
}
