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

/* Event parser */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "gsx_parse.h"

enum {
	ST_NORMAL,
	ST_ATOM, /* unquoted atom */
	ST_DQ,   /* in double quote */
	ST_SQ,   /* in single quote */
	ST_BRC,  /* in brace */
	ST_DQBS, /* in double quote, previous char was a backslash */
	ST_SQBS, /* in single quote, previous char was a backslash */
	ST_QEND, /* quote end - need to be followed by a whitepsace or ) */
	ST_ERR,  /* error condition (stopped parsing) */
	ST_EOE   /* end-of-file condition (stopped parsing) */
};

static void update_loc(gsx_parse_t *ctx, int chr)
{
	ctx->offs++;
	if (chr == '\n') {
		ctx->col = 0;
		ctx->line++;
	}
	else
		ctx->col++;
}

#define error(ctx, chr, errmsg) \
do { \
	ctx->cb(ctx, GSX_EV_ERROR, errmsg); \
	ctx->pstate = ST_ERR; \
	update_loc(ctx, chr); \
	return GSX_RES_ERROR; \
} while(0)

static void flush_atom(gsx_parse_t *ctx)
{
	ctx->atom[ctx->used] = '\0';
	ctx->cb(ctx, GSX_EV_ATOM, ctx->atom);
	ctx->used = 0;
}

int gsx_parse_char(gsx_parse_t *ctx, int chr)
{
	int qc, stbs;

	if (ctx->pstate == ST_ERR) {
		update_loc(ctx, chr);
		return GSX_RES_ERROR;
	}

	if (ctx->pstate == ST_EOE) {
		update_loc(ctx, chr);
		return GSX_RES_EOE;
	}

	if (chr == EOF) /* we are not in ST_EOE if we got here */
			error(ctx, chr, "premature end of expression");

	if ((ctx->line_comment_char != 0) && (chr == ctx->line_comment_char) && ctx->last_newline)
		ctx->in_comment = 1;
	ctx->last_newline = (chr == '\r') || (chr == '\n');
	if (ctx->last_newline && ctx->in_comment) {
		ctx->in_comment = 0;
		goto skip_comment;
	}
	if (ctx->in_comment)
		goto skip_comment;

	switch(ctx->pstate) {
		case ST_DQBS:
			ctx->pstate = ST_DQ;
			goto append;

		case ST_SQBS:
			ctx->pstate = ST_SQ;
			goto append;

		case ST_DQ:
			qc = '"';
			stbs = ST_DQBS;
			goto quote;

		case ST_SQ:
			qc = '\'';
			stbs = ST_SQBS;
			quote:;
			if (chr == qc)
				ctx->pstate = ST_QEND;
			else if (chr == '\\')
				ctx->pstate = stbs;
			else
				goto append;
			break;

		case ST_BRC:
			stbs = ST_BRC;
			qc = '}';
			goto quote;

		case ST_QEND:
			if (isspace(chr)) {
				if (ctx->depth == 0)
					goto err_atom0;
				ctx->pstate = ST_NORMAL;
				flush_atom(ctx);
				break;
			}
			if (chr == ')') {
				if (ctx->depth == 0)
					goto err_atom0;
				flush_atom(ctx);
				goto close;
			}

			if (chr == '(') {
				flush_atom(ctx);
				goto open;
			}

			/* some dialects allow missing space after quote close - in which case we need to glue the atoms */
			ctx->pstate = ST_ATOM;
			goto append;

		case ST_ATOM:
			if (isspace(chr)) {
				if (ctx->depth == 0)
					goto err_atom0;
				ctx->pstate = ST_NORMAL;
				flush_atom(ctx);
				break;
			}
			if (chr == ')') {
				if (ctx->depth == 0)
					goto err_atom0;
				flush_atom(ctx);
				goto close;
			}
			if (chr == '"') {
				/* glue "foo"bar to become foobar */
				qc = '"';
				stbs = ST_DQBS;
				ctx->pstate = ST_DQ;
				break;
			}
			if ((chr == '(') || (chr == '\''))
				error(ctx, chr, "unquoted atom with special character in it");
			goto append;

		case ST_NORMAL:
			if (isspace(chr))
				break; /* excess whitepsace between atoms */

			switch(chr) {
				case '"':
					ctx->pstate = ST_DQ;
					break;

				case '\'':
					ctx->pstate = ST_SQ;
					break;

				case '{':
					if (ctx->brace_quote) {
						ctx->pstate = ST_BRC;
						break;
					}
					ctx->pstate = ST_ATOM;
					goto append;

				case '(':
					open:;
					ctx->cb(ctx, GSX_EV_OPEN, "(");
					ctx->depth++;
					break;

				case ')':
					close:;
					ctx->depth--;
					ctx->cb(ctx, GSX_EV_CLOSE, ")");
					if (ctx->depth == 0) {
						ctx->pstate = ST_EOE;
						update_loc(ctx, chr);
						return GSX_RES_EOE;
					}
					ctx->pstate = ST_NORMAL;
					break;

				default: /* normal character - start of a new, unquoted atom */
					ctx->pstate = ST_ATOM;
					goto append;
			}
			break;
			
	}

	skip_comment:;
	update_loc(ctx, chr);
	return GSX_RES_NEXT;

	append:;
	if (ctx->used >= ctx->alloced) {
		/* growth strategy */
		if (ctx->alloced < 1024)
			ctx->alloced += 128;
		else if (ctx->alloced > 1024*1024)
			ctx->alloced += 1024*1024;
		else
			ctx->alloced *= 2;

		/* realloc, make sure there's 1 byte of extra room for the \0 */
		ctx->atom = realloc(ctx->atom, ctx->alloced+1);
	}
	ctx->atom[ctx->used] = chr;
	ctx->used++;
	update_loc(ctx, chr);
	return GSX_RES_NEXT;

	err_atom0:
	error(ctx, ' ', "Expression not wrapped in ()");
}


void gsx_parse_init(gsx_parse_t *ctx)
{
	ctx->offs = ctx->line = ctx->col = ctx->depth = 0;

	ctx->atom = NULL;
	ctx->used = ctx->alloced = 0;
	ctx->pstate = ST_NORMAL;
	ctx->in_comment = 0;
	ctx->last_newline = 0;
}

void gsx_parse_uninit(gsx_parse_t *ctx)
{
	free(ctx->atom);
	ctx->atom = NULL;
	ctx->used = ctx->alloced = 0;
}
