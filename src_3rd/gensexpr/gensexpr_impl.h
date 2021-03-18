#ifndef GENSEXPR_H
#define GENSEXPR_H

#include <stdlib.h>

#include <gensexpr/gsx_parse.h>

typedef struct GSX(node_s) GSX(node_t);
typedef struct GSX(dom_s)  GSX(dom_t);

struct GSX(node_s) {
	char *str;
	GSX(node_t) *parent, *children, *next;
#ifdef GENSEXPR_WANT_LOC
	size_t offs, line, col;
#endif
};

struct GSX(dom_s) {
	GSX(node_t) *root;
	size_t node_size; /* including gsx_node_t and user data */

	/* dump configuration */
	unsigned dump_allow_sq:1; /* whether to allow using single quote */
	unsigned dump_allow_dq:1; /* whether to allow using single quote */

	/* optional custom allocator */
	void *(*malloc)(GSX(dom_t) *dom, size_t size);
	void (*free)(GSX(dom_t) *dom, void *data);

	void *user_ctx;     /* optional: used by the caller */

	/* Internal */
	gsx_parse_t parse;
	GSX(node_t) *parse_current;
};

/* Note: the init/alloc macros are in the implementation headers */
void GSX(uninit)(GSX(dom_t) *dom);
void GSX(free)(GSX(dom_t) *dom);

gsx_parse_res_t GSX(parse_char)(GSX(dom_t) *dom, int chr);

/* This function must be called if the parse is cancelled before the root is
   closed, else the tree is corrupt. */
void GSX(cancel_parse)(GSX(dom_t) *dom);

void GSX(dump_subtree)(GSX(dom_t) *dom, GSX(node_t) *node, void (*write)(void *wctx, const char *str), void *wctx);
void GSX(dump_tree)(GSX(dom_t) *dom, void (*write)(void *wctx, const char *str), void *wctx);

void GSX(node_free)(GSX(dom_t) *dom, GSX(node_t) *node);

void GSX(compact_subtree)(GSX(dom_t) *dom, GSX(node_t) *nd);
void GSX(compact_tree)(GSX(dom_t) *dom);

GSX(node_t) *GSX(nth)(GSX(node_t) *node, int idx);

/* low level / internal use */
void GSX(init_)(GSX(dom_t) *dom, size_t node_size);
GSX(dom_t) *GSX(alloc_)(size_t node_size);

#endif
