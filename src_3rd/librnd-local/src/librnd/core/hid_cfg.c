/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* low level lihata manipulation for librnd config and menu files */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <liblihata/lihata.h>
#include <liblihata/tree.h>

#include <librnd/config.h>
#include <librnd/core/hid.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/core/error.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/hidlib.h>

char hid_cfg_error_shared[1024];

static int hid_cfg_load_error(lht_doc_t *doc, const char *filename, lht_err_t err)
{
	const char *fn;
	int line, col;
	lht_dom_loc_active(doc, &fn, &line, &col);
	rnd_message(RND_MSG_ERROR, "Resource error: %s (%s:%d.%d)*\n", lht_err_str(err), filename, line+1, col+1);
	return 1;
}

lht_doc_t *rnd_hid_cfg_load_lht(rnd_hidlib_t *hidlib, const char *filename)
{
	FILE *f;
	lht_doc_t *doc;
	int error = 0;

	f = rnd_fopen(hidlib, filename, "r");
	if (f == NULL)
		return NULL;

	doc = lht_dom_init();
	lht_dom_loc_newfile(doc, filename);

	while(!(feof(f))) {
		lht_err_t err;
		int c = fgetc(f);
		err = lht_dom_parser_char(doc, c);
		if (err != LHTE_SUCCESS) {
			if (err != LHTE_STOP) {
				error = hid_cfg_load_error(doc, filename, err);
				break;
			}
			break; /* error or stop, do not read anymore (would get LHTE_STOP without any processing all the time) */
		}
	}

	if (error) {
		lht_dom_uninit(doc);
		doc = NULL;
	}
	fclose(f);

	return doc;
}

lht_doc_t *rnd_hid_cfg_load_str(const char *text)
{
	lht_doc_t *doc;
	int error = 0;

	doc = lht_dom_init();
	lht_dom_loc_newfile(doc, "embedded");

	while(*text != '\0') {
		lht_err_t err;
		int c = *text++;
		err = lht_dom_parser_char(doc, c);
		if (err != LHTE_SUCCESS) {
			if (err != LHTE_STOP) {
				error = hid_cfg_load_error(doc, "internal", err);
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

const char *rnd_hid_cfg_text_value(lht_doc_t *doc, const char *path)
{
	lht_node_t *n = lht_tree_path(doc, "/", path, 1, NULL);
	if (n == NULL)
		return NULL;
	if (n->type != LHT_TEXT) {
		rnd_hid_cfg_error(n, "Error: node %s should be a text node\n", path);
		return NULL;
	}
	return n->data.text.value;
}

/************* "parsing" **************/

void rnd_hid_cfg_extend_hash_nodev(lht_node_t *node, va_list ap)
{
	for(;;) {
		char *cname, *cval;
		lht_node_t *t, *li;

		cname = va_arg(ap, char *);
		if (cname == NULL)
			break;
		cval = va_arg(ap, char *);

		if (cval == NULL)
			continue;

		if (strncmp(cname, "li:", 3) == 0) {
			char *start, *s, *tmp = rnd_strdup(cval);
			li = lht_dom_node_alloc(LHT_LIST, cname+3);

			for(s = start = tmp; *s != '\0'; s++) {
				if (*s == '\\') {
					s++;
					if (*s == '\0')
						break;
				}
				if (*s == ';') {
					*s = '\0';
					t = lht_dom_node_alloc(LHT_TEXT, "");
					t->data.text.value = rnd_strdup(start);
					lht_dom_list_append(li, t);
					start = s+1;
				}
			}
			if (*start != '\0') {
				t = lht_dom_node_alloc(LHT_TEXT, "");
				t->data.text.value = rnd_strdup(start);
				lht_dom_list_append(li, t);
			}
			free(tmp);
		}
		else {
			t = lht_dom_node_alloc(LHT_TEXT, cname);
			t->data.text.value = rnd_strdup(cval);
			lht_dom_hash_put(node, t);
		}
	}
}

void rnd_hid_cfg_extend_hash_node(lht_node_t *node, ...)
{
	va_list ap;
	va_start(ap, node);
	rnd_hid_cfg_extend_hash_nodev(node, ap);
	va_end(ap);
}

lht_node_t *rnd_hid_cfg_create_hash_node(lht_node_t *parent, lht_node_t *ins_after, const char *name, ...)
{
	lht_node_t *n;
	va_list ap;

	if ((parent != NULL) && (parent->type != LHT_LIST))
		return NULL;

	/* ignore ins_after if we are already deeper in the tree */
	if ((ins_after != NULL) && (ins_after->parent != parent))
		ins_after = NULL;

	n = lht_dom_node_alloc(LHT_HASH, name);
	if (ins_after != NULL) {
		/* insert as next sibling below a @anchor */
		lht_dom_list_insert_after(ins_after, n);
	}
	else if (parent != NULL) {
		/* insert as last item under a parent node */
		lht_dom_list_append(parent, n);
	}

	va_start(ap, name);
	rnd_hid_cfg_extend_hash_nodev(n, ap);
	va_end(ap);

	return n;
}

int rnd_hid_cfg_dfs(lht_node_t *parent, int (*cb)(void *ctx, lht_node_t *n), void *ctx)
{
	lht_dom_iterator_t it;
	lht_node_t *n;

	for(n = lht_dom_first(&it, parent); n != NULL; n = lht_dom_next(&it)) {
		int ret;
		ret = cb(ctx, n);
		if (ret != 0)
			return ret;
		if (n->type != LHT_TEXT) {
			ret = rnd_hid_cfg_dfs(n, cb, ctx);
			if (ret != 0)
				return ret;
		}
	}
	return 0;
}

void rnd_hid_cfg_error(const lht_node_t *node, const char *fmt, ...)
{
	char *end;
	va_list ap;
	int len, maxlen = sizeof(hid_cfg_error_shared);

	len = rnd_snprintf(hid_cfg_error_shared, maxlen, "Error in lihata node %s:%d.%d:", node->file_name, node->line, node->col);
	end = hid_cfg_error_shared + len;
	maxlen -= len;
	va_start(ap, fmt);
	end += rnd_vsnprintf(end, maxlen, fmt, ap);
	va_end(ap);
	rnd_message(RND_MSG_ERROR, hid_cfg_error_shared);
}
