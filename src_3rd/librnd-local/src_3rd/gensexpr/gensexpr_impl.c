/*
Copyright (c) 2016 Tibor 'Igor2' Palinkas
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of the Author nor the names of contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

Contact: gensexpr {at} igor2.repo.hu
Project page: http://repo.hu/projects/gensexpr
*/

#include <string.h>

static void *gsx_dummy_malloc(GSX(dom_t) *dom, size_t size)
{
	return malloc(size);
}

static void gsx_dummy_free(GSX(dom_t) *dom, void *data)
{
	free(data);
}

#define alloc_node(ctx) ctx->malloc(ctx, ctx->node_size)

/* Abuse the str field for storing the last children for fast append */
#define last(nd) (nd)->str

static char *gsx_strdup(const char *s)
{
	int len = strlen(s)+1;
	char *res;
	res = malloc(len);
	memcpy(res, s, len);
	return res;
}

#ifdef GENSEXPR_WANT_LOC
static void setloc(GSX(node_t) *nd, gsx_parse_t *parse) {
	nd->offs = parse->offs;
	nd->line = parse->line;
	nd->col = parse->col;
}
#else
static void setloc(GSX(node_t) *nd, gsx_parse_t *parse) { }
#endif

/* if a parsing is cancelled in the middle, we have a random number of tree
   nodes abusing their str fields for keeping track of last children. This
   would cause double-free on uninit() so reset these strings to NULL. Any
   node with children != NULL will have a last(), and won't have a real str */
static void undo_last(GSX(node_t) *nd)
{
	GSX(node_t) *n;
	last(nd) = NULL;
	for(n = nd->children; n != NULL; n = n->next)
		if (n->children != NULL)
			undo_last(n);
}

static void cancel_parse(gsx_parse_t *pctx)
{
	GSX(dom_t) *ctx = (GSX(dom_t) *)pctx->user_ctx;
	if ((ctx->root != NULL) && (ctx->root->children != NULL))
		undo_last(ctx->root);
}

static void GSX(parser_ev)(gsx_parse_t *pctx, gsx_parse_event_t ev, const char *data)
{
	GSX(dom_t) *ctx = (GSX(dom_t) *)pctx->user_ctx;
	GSX(node_t) *nd;

	switch(ev) {
		case GSX_EV_OPEN:
			if (ctx->root == NULL) {
				ctx->root = ctx->parse_current = alloc_node(ctx);
				memset(ctx->root, 0, sizeof(GSX(node_t)));
				setloc(ctx->root, &ctx->parse);
			}
			else {
				nd = alloc_node(ctx);
				memset(nd, 0, sizeof(GSX(node_t)));
				setloc(nd, &ctx->parse);
				nd->parent = ctx->parse_current;
				ctx->parse_current = nd;
				goto append;
			}
			break;
		case GSX_EV_CLOSE:
			last(ctx->parse_current) = NULL;
			ctx->parse_current = ctx->parse_current->parent;
			break;

		case GSX_EV_ATOM:
			nd = alloc_node(ctx);
			memset(nd, 0, sizeof(GSX(node_t)));
			setloc(nd, &ctx->parse);
			nd->parent = ctx->parse_current;
			nd->str = gsx_strdup(data);

			append:;
			/* append at parent - single linked list with a pointer to the end (last()) */
			if (nd->parent == NULL)
				break;
			if (nd->parent->children != NULL)
				((GSX(node_t *))(last(nd->parent)))->next = nd;
			else
				nd->parent->children = nd;
			last(nd->parent) = (char *)nd;
			break;

		case GSX_EV_ERROR:
			cancel_parse(pctx);
			break;
	}
}

void GSX(init_)(GSX(dom_t) *dom, size_t node_size)
{
	dom->root = NULL;
	dom->node_size = node_size;
	dom->malloc = gsx_dummy_malloc;
	dom->free = gsx_dummy_free;

	dom->dump_allow_dq = 1;
	dom->dump_allow_sq = 0;

	gsx_parse_init(&dom->parse);
	dom->parse.line_comment_char = '\0';
	dom->parse.user_ctx = dom;
	dom->parse.cb = GSX(parser_ev);
	dom->parse_current = NULL;
}

GSX(dom_t) *GSX(alloc_)(size_t node_size)
{
	GSX(dom_t) *dom = malloc(sizeof(GSX(dom_t)));
	if (dom == NULL)
		return NULL;
	GSX(init_)(dom, node_size);
	return dom;
}

void GSX(node_free)(GSX(dom_t) *dom, GSX(node_t) *node)
{
	GSX(node_t) *n, *next;
	for(n = node->children; n != NULL; n = next) {
		next = n->next;
		GSX(node_free)(dom, n);
	}
	dom->free(dom, node->str);
	dom->free(dom, node);
}

void GSX(uninit)(GSX(dom_t) *dom)
{
	if (dom->root != NULL)
		GSX(node_free)(dom, dom->root);
	gsx_parse_uninit(&dom->parse);
}

void GSX(free)(GSX(dom_t) *dom)
{
	GSX(uninit)(dom);
	free(dom);
}

gsx_parse_res_t GSX(parse_char)(GSX(dom_t) *dom, int chr)
{
	return gsx_parse_char(&dom->parse, chr);
}

void GSX(cancel_parse)(GSX(dom_t) *dom)
{
	cancel_parse(&dom->parse);
}

void GSX(dump_subtree)(GSX(dom_t) *dom, GSX(node_t) *node, void (*write)(void *wctx, const char *str), void *wctx)
{
	int compact = (node->str != NULL) && (node->children != NULL);

	/* Special case: empty terminal node */
	if ((node->str == NULL) && (node->children == NULL)) {
		write(wctx, "()");
		return;
	}

	if (compact)
		write(wctx, "(");

	if (node->str != NULL) {
		int has_sq = 0, has_dq = 0, has_spec = 0;
		char *s;

		for(s = node->str; *s != '\0'; s++) {
			switch(*s) {
				case '\'': has_sq = 1; continue;
				case '"':  has_dq = 1; continue;
				case ')':  has_spec = 1; continue;
			}
			if ((*s > ' ') && (*s < 127))
				continue;
			has_spec = 1;
		}

		if (!has_sq && !has_dq && !has_spec) /* safe */
			write(wctx, node->str);
		else if ((!has_dq) && (dom->dump_allow_dq)) { /* try double quote */
			write(wctx, "\"");
			write(wctx, node->str);
			write(wctx, "\"");
		}
		else if ((!has_sq) && (dom->dump_allow_sq)) { /* try double quote */
			write(wctx, "\'");
			write(wctx, node->str);
			write(wctx, "\'");
		}
		else { /* all fail - have to use quote and escaping */
			char qc[2], tmp;
			char *s, *from;
			if (dom->dump_allow_dq) *qc = '"';
			else if (dom->dump_allow_sq) *qc = '\'';
			else *qc = 0;
			
			if (*qc != 0) {
				qc[1] = '\0';
				write(wctx, qc);
			}
			for(from = s = node->str; *s != '\0'; s++) {
				if ((*s == *qc) || (*s < ' ') || (*s == '\\') || (*s >= 127)) {
					if (s > from) {
						tmp = *s;
						*s = '\0';
						write(wctx, from);
						*s = tmp;
					}
					write(wctx, "\\");
					from = s;
					s++;
				}
			}
			if (s > from)
				write(wctx, from);
			if (*qc != 0)
				write(wctx, qc);
		}
	}

	if (node->children != NULL) {
		GSX(node_t) *n;
		int i;

		if (!compact)
			write(wctx, "(");
		for(i = compact, n = node->children; n != NULL; i++, n = n->next) {
			if (i != 0)
				write(wctx, " ");
			GSX(dump_subtree)(dom, n, write, wctx);
		}
		if (!compact)
			write(wctx, ")");
	}

	if (compact)
		write(wctx, ")");

}

void GSX(dump_tree)(GSX(dom_t) *dom, void (*write)(void *wctx, const char *str), void *wctx)
{
	if (dom->root != NULL)
		GSX(dump_subtree)(dom, dom->root, write, wctx);
}

void GSX(compact_subtree)(GSX(dom_t) *dom, GSX(node_t) *nd)
{
	GSX(node_t) *old_ch, *n;

	for(n = nd->children; n != NULL; n = n->next)
		GSX(compact_subtree)(dom, n);

	if ((nd->str == NULL) && (nd->children != NULL) && (nd->children->str != NULL)) {
		old_ch = nd->children;
		nd->str = nd->children->str;
		nd->children = old_ch->next;
		for(n = old_ch->next; n != NULL; n = n->next)
			n->parent = nd;

		old_ch->str = NULL;
		old_ch->next = NULL;
		dom->free(dom, old_ch);
	}
}

void GSX(compact_tree)(GSX(dom_t) *dom)
{
	if (dom->root != NULL)
		GSX(compact_subtree)(dom, dom->root);
}

GSX(node_t) *GSX(nth)(GSX(node_t) *node, int idx)
{
	if ((idx == 0) && (node->str != NULL)) /* compact tree has 0th arg in node */
		return node;

	node = node->children;

	if (node->str != NULL) /* compact tree has 0th arg in node */
		idx--;

	while(idx > 0) {
		if (node == NULL)
			return NULL;
		idx--;
		node = node->next;
	}

	return node;
}
