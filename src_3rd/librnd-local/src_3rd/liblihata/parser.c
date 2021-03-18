/*
    liblihata - list/hash/table format, parser lib
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


   This file provides generic API for the most basic operations common to
   the event parser and the DOM parser.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"


#if 0
#define TRACE fprintf(stderr, "%d '%c'/%d ty=%d\n", sp->state, c, c, sp->type)
#else
#define TRACE
#endif

#define grow_(out, arr, used, alloced, type, growth) \
	do { \
		if ((used) >= (alloced)) { \
			(alloced) += (growth); \
			(arr) = realloc((arr), sizeof(type) * (alloced)); \
		} \
		(out) = (arr)+(used); \
		(used)++; \
	} while(0)

#define grow_stack(out) grow_(out, ctx->stack, ctx->sused, ctx->salloced, lht_pstack_t, 32)
#define grow_token(out) grow_(out, ctx->token, ctx->tused, ctx->talloced, char, 256)

/* append a character to the current token */
#define append(c) \
	do { \
		char *s; \
		grow_token(s); \
		*s = c; \
	} while(0)

#define delete()  ctx->tused=0

/* push new state on top of the stack */
#define push2(new_state, new_type, new_str, expl_name, expl_type) \
	do { \
		lht_pstack_t *_push_sp; \
		grow_stack(_push_sp); \
		_push_sp->state = new_state; \
		_push_sp->type = new_type; \
		_push_sp->explicit_type = expl_type; \
		_push_sp->explicit_name = expl_name; \
		if (new_str == NULL) \
			_push_sp->str = NULL; \
		else \
			_push_sp->str = lht_strdup(new_str); \
	} while(0)

/* Can not call push2 without an advanced gcc warning about strdup getting NULL */
#define push(new_state, new_type) \
	do { \
		lht_pstack_t *_push_sp; \
		grow_stack(_push_sp); \
		_push_sp->state = new_state; \
		_push_sp->type = new_type; \
		_push_sp->str = NULL; \
		_push_sp->explicit_type = 0; \
		_push_sp->explicit_name = 0; \
	} while(0)

/* low level pop: remove/free topmost entry without any checks or error reporting */
#define pop_(_pop_sp) \
	do { \
		if (_pop_sp->str != NULL) \
			free(_pop_sp->str);\
		ctx->sused--; \
	} while(0);

/* high level pop: remove top (current) state and return to previous;
   set outsp to the previous state as well. Report error on underflow. */
#define pop(outsp) \
	do { \
		lht_pstack_t *_pop_sp = ctx->stack+ctx->sused-1; \
		pop_(_pop_sp); \
		if (ctx->sused < 0) \
			error(LHTE_STACK_UNDERFLOW); \
		outsp = ctx->stack+ctx->sused-1; \
	} while(0)

/* parse error */
#define error(code) \
	do { \
		ctx->event(ctx, LHT_ERROR, LHT_INVALID_TYPE, lht_err_str(code), NULL); \
		sp->state = LHT_ST_DEAD; \
		return code; \
	} while(0)

/* state stack: new nodes will get their own entry that is popped when the node ends */
struct lht_pstack_s {
	lht_pstate_t state;
	lht_node_type_t type;
	char *str;
	unsigned explicit_type:1;
	unsigned explicit_name:1;
};


void lht_parser_init(lht_parse_t *ctx)
{
	/* reset only lihata part of the struct */
	memset(&(ctx->line), 0, sizeof(lht_parse_t) - ((char *)&(ctx->line) - (char *)ctx));
	push(LHT_ST_BODY, LHT_TEXT);
}

void lht_parser_uninit(lht_parse_t *ctx)
{
	while(ctx->sused > 0) {
		lht_pstack_t *sp = ctx->stack+ctx->sused-1;
		pop_(sp);
	}

	if (ctx->token != NULL)
		free(ctx->token);

	if (ctx->stack != NULL)
		free(ctx->stack);

	/* defensive: just in case the ctx is still reused somehow, this guarantees a crash on null pointer access which is better than silently using an already uninitialized parser */
	ctx->salloced = ctx->talloced = 16;
	ctx->sused = ctx->tused = 0;
}

static lht_node_type_t type_lookup(char c1, char c2)
{
	if ((c1 == 't') && (c2 == 'e')) return LHT_TEXT;
	if ((c1 == 'l') && (c2 == 'i')) return LHT_LIST;
	if ((c1 == 'h') && (c2 == 'a')) return LHT_HASH;
	if ((c1 == 't') && (c2 == 'a')) return LHT_TABLE;
	if ((c1 == 's') && (c2 == 'y')) return LHT_SYMLINK;
	return LHT_INVALID_TYPE;
}

/* strip trailing white space from token */
static void tstrip_trailing_ws(lht_parse_t *ctx)
{
	/* array instead of pointer - to avoid ptr-- at the end that would point before the array in extreme case, which is UB in C99 */
	while((ctx->tused > 0) && ((ctx->token[ctx->tused - 1] == ' ') || (ctx->token[ctx->tused - 1] == '\t')))
		ctx->tused--;
}

#define update_sp   sp = ctx->stack + ctx->sused - 1;
#define is_text(ty)  (((ty) == LHT_TEXT) || ((ty) == LHT_SYMLINK))
#define in_root(ctx) ((ctx)->sused == 1)

#define set_default_implicit_type(sp_SDIT, ptype) \
do { \
 ((sp_SDIT)->type) = (ptype == LHT_TABLE ? LHT_LIST : LHT_TEXT); \
	(sp_SDIT)->explicit_type = 0; \
} while(0)

#define newline_admin() \
	do { \
		ctx->col = 0; \
		ctx->line++; \
	} while(0)

/* insert a character in the stream while keeping proper line/col numbering */
#define insert_char(c) \
	do { \
		int oldl = ctx->line; \
		int oldc = ctx->col; \
		lht_parser_char(ctx, c); \
		ctx->col = oldc; \
		ctx->line = oldl; \
	} while(0)


lht_err_t lht_parser_char(lht_parse_t *ctx, int c)
{
	lht_pstack_t *sp;

	update_sp;
	TRACE;
	ctx->col++;

	if (c == '\0')
		error(LHTE_NUL_CHAR);
	switch(sp->state) {
		case LHT_ST_DEAD: return LHTE_STOP; /* Hah, kicking the dead horse? */
		case LHT_ST_BACKSLASH: /* next character is literal */
			if (c == EOF)
				error(LHTE_EOF_BACKSLASH); /* can not escape the EOF */
			append(c);
			pop(sp);
			break;
		case LHT_ST_COMMENT: /* comment: read line up to \n */
			switch(c) {
				case '\n':
					newline_admin();
				case '\r':
				case EOF:
					append('\0');
					ctx->event(ctx, LHT_COMMENT, LHT_INVALID_TYPE, ctx->token, NULL);
					delete();
					pop(sp);
					if (c == EOF)
						goto valid_eof; /* last line being a comment, without newline at the end */
					break;
				default:
					append(c);
			}
			break;
		case LHT_ST_BSTRING:    /* brace string: read until the closing } */
			switch(c) {
				case EOF:
					error(LHTE_EOF_BSTRING);
				case '\\':
					push(LHT_ST_BACKSLASH, LHT_INVALID_TYPE);
					break;
				case '}':
					pop(sp);
					switch(sp->state) {  /* there are only two contexts for a brace string: */
						case LHT_ST_BODY: goto finish_nodename_braced;
						case LHT_ST_PREVALUE: goto got_string;
						default:
							error(LHTE_INV_STATE_BSTRING);
					}
				default:
					append(c);
			}
			break;
		case LHT_ST_BODY:
			switch(c) {
				case EOF:
					goto valid_eof; /* eof in body is valid if body is above the root */
				case '\n':
					newline_admin();
				case '\r':
				case ';':
					/* separator in body means the end of a non-brace-protected string */
					if (ctx->tused > 0) {
						push(LHT_ST_TVALUE, sp->type); /* got_string will pop */
						update_sp;
						tstrip_trailing_ws(ctx);
						goto got_string;
					}
					/* ignore extra separators */
					break;
				case ' ':
				case '\t':
					if (ctx->tused > 0)
						append(c);
					/* ... or ignore leading whitespace */
					break;
				case '\\':
					push(LHT_ST_BACKSLASH, LHT_INVALID_TYPE);
					break;
				case '=':
					tstrip_trailing_ws(ctx);
					if (is_text(sp->type))
						sp->explicit_name = 1;
					goto finish_nodename;
				case '}':
					/* end of a brace enclosed value list */
					insert_char('\n');  /* insert a separator before the '}', closing any open record allowing single-liners */
					pop(sp);
					ctx->event(ctx, LHT_CLOSE, LHT_INVALID_TYPE, NULL, NULL);
					if (ctx->sused <= 1) /* closing the root node means: stop parsing */
						goto implicit_eof;
					else
						set_default_implicit_type(sp, sp[-1].type); /* if this node is not the root, set back implicit type as parent requires */
					break;
				case '{': /* "{" in bstring means a protected node name or anonymous string value, if we already have a name */
					if ((ctx->tused != 0) && (is_text(sp->type))) { /* non-empty name and brace means a brace-protected value */
							tstrip_trailing_ws(ctx);
							goto finish_nodename_start_bstring;
					}
					else if (is_text(sp->type)) /* empty name and brace-open means it's going to be a string protection; whether it's a name or a value will be figured at the end */
						push(LHT_ST_BSTRING, LHT_INVALID_TYPE);
					else
						goto finish_nodename_start_body; /* not a text node, not a symlink, still does not have a name - anonymous node with body */
					break;
				case ':':
					if ((ctx->tused == 2) && (sp->explicit_type == 0)) { /* unprotected colon in the 3rd character is a type separator */
						sp->type = type_lookup(ctx->token[0], ctx->token[1]);
						if (sp->type == LHT_INVALID_TYPE)
							error(LHTE_INVALID_TYPE);
						sp->explicit_type = 1;
						delete();
					}
					else
						error(LHTE_COLON); /* do not allow unprotected colons */
					break;
				case '#':
					push(LHT_ST_COMMENT, LHT_INVALID_TYPE);
					break;
				default:
					append(c);
			}
			break;
		case LHT_ST_PREVALUE:
			switch(c) {
				case EOF:
					/* corner case: anonymous text in root, without termination - we wouldn't be able to decide whether it's a valid EOF or the file is truncated; better to require the termination */
					error(LHTE_EOF_VALUE); /* EOF while waiting for value is bad */
				case '\n':
					newline_admin();
				case '\r':
				case ';':
					/* text node with only one text (with or without =)*/
					if (is_text(sp->type))
						tstrip_trailing_ws(ctx);
					goto got_string;
				case '=':
					sp->explicit_name = 1; /* for the case of {key}={} */
				case ' ':
				case '\t':
					break; /* ignore whitespace between name and value */
				case '{':
					if (is_text(sp->type)) { /* for a text value brace is protection */
						delete();
						push(LHT_ST_BSTRING, LHT_INVALID_TYPE);
					}
					else { /* for anything else it's just BODY */
						ctx->event(ctx, LHT_OPEN, sp->type, sp->str, NULL);
						sp->state = LHT_ST_BODY;
						set_default_implicit_type(sp, sp[-1].type);
					}
					break;
				case '\\':
					if (is_text(sp->type)) { /* text can start with an escaped character */
						push(LHT_ST_TVALUE, sp->type);
						push(LHT_ST_BACKSLASH, LHT_INVALID_TYPE);
					}
					else /* but nothing else can - we need a "{" */
						error(LHTE_ESCALHTE_NONTEXT);
					break;
				case '}':
					error(LHTE_INV_TEXT);
				default:
					if (is_text(sp->type)) { /* anything else is already a text value for text ... */
						append(c);
						push(LHT_ST_TVALUE, sp->type);
					}
					else /* or an error since we expect "{" */
						error(LHTE_INVALID_CHAR_PREVALUE);
			}
			break;
		case LHT_ST_TVALUE: /* simple, unprotected text value */
			switch(c) {
				case EOF:
					error(LHTE_EOF_TVALUE); /* text value is requires valid termination, otherwise we wouldn't be able to decide if it's truncated */
				case '\\':
					push(LHT_ST_BACKSLASH, LHT_INVALID_TYPE);
					break;
				case '\n':
					newline_admin();
				case '\r':
				case ';':
					pop(sp);
					append('\0');
					if (is_text(sp->type)) {
						tstrip_trailing_ws(ctx);
						goto got_string;
					}
					error(LHTE_INV_STATE_TVALUE); /* TVALUE in non-text-type?! */
				case '}':
					/* unprotected } in string means closing of the parent node; cheapest solution is to "fix it" by inserting a \n */
					insert_char('\n');
					insert_char('}');
					break;
				case '{':
				case '=':
				case ':':
					error(LHTE_INV_TEXT); /* these should be protected */
					break;
				default:
					append(c);
			}
	}

	return LHTE_SUCCESS;

finish_nodename_braced:;
	if ((ctx->tused > 2) && (ctx->token[2] == ':')) { /* token long enough to have an explicit type */
		lht_node_type_t ty = type_lookup(ctx->token[0], ctx->token[1]);
		if (ty != LHT_INVALID_TYPE) { /* braced name with a valid type: strip type */
			append('\0');
			sp->type = ty;
			sp->explicit_name = 1;
			sp->explicit_type = 1;
			push2(LHT_ST_PREVALUE, sp->type, ctx->token+3, sp->explicit_name, sp->explicit_type);
			delete();
			return LHTE_SUCCESS;
		}
	}
	/* else assume implicit "te:", because we didn't have a valid type prefix: */

finish_nodename:;
	append('\0');
	push2(LHT_ST_PREVALUE, sp->type, ctx->token, sp->explicit_name, 0);
	delete();
	return LHTE_SUCCESS;
finish_nodename_start_body:;
finish_nodename_start_bstring:;
	insert_char('=');
	insert_char('{');
	return LHTE_SUCCESS;
valid_eof:;
	ctx->col--; /* eof is not really a character, didn't add to column */
	if (!in_root(ctx))
		error(LHTE_EOF_NODE);
implicit_eof:;
	ctx->event(ctx, LHT_EOF, LHT_INVALID_TYPE, NULL, NULL);
	sp->state = LHT_ST_DEAD;
	return LHTE_STOP;
got_string:;
	append('\0');

	if (sp->explicit_name) { /* we do have a name */
		if (*ctx->token == '\0')
			ctx->event(ctx, LHT_TEXTDATA, sp->type & 0x7f,  sp->str, ""); /* no token means empty node */
		else
			ctx->event(ctx, LHT_TEXTDATA, sp->type & 0x7f,  sp->str, ctx->token);
	}
	else if (is_text(sp->type)) { /* we do not have a name for sure */
		if (*ctx->token == '\0')
			ctx->event(ctx, LHT_TEXTDATA, sp->type, "", sp->str); /* empty token means the one we had earlier must be an anonymous text content */
		else
			ctx->event(ctx, LHT_TEXTDATA, sp->type, sp->str == NULL ? "" : sp->str, ctx->token);
	}
	else
		error(LHTE_NO_BODY);
	delete();
	pop(sp);
	set_default_implicit_type(sp, sp->type);

	/* restore implicit node to anonymous */
	sp->explicit_name = 0;
	return LHTE_SUCCESS;
}

lht_pstate_t lht_parser_get_state(lht_parse_t *ctx, int *explicit_type, int *explicit_name)
{
	lht_pstack_t *sp;
	update_sp;
	if (explicit_type != NULL)
		*explicit_type = sp->explicit_type;
	if (explicit_name != NULL)
		*explicit_name = sp->explicit_name;
	return sp->state;
}

