/*
    scconfig - hook readline lib
    Copyright (C) 2016   Tibor Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

		Project page: http://repo.hu/projects/scconfig
		Contact via email: scconfig [at] igor2.repo.hu
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "hookreadline.h"

#define trprintf(...)

#define hrl_puts(h, s)       h->write(h, s, strlen(s))

#define query_curpos(h) \
	vt100_where(&h->term)

#define update_curspos(h) \
	do {\
		if ((h->term.ccol <= 1) || (h->term.ccol >= h->term.maxcol) || (h->term.crow >= h->term.maxrow)) { \
			if (!h->certain_cpos) {\
				trprintf("q1\n"); \
				query_curpos(h); \
				h->queried_pos = 1; \
			} \
		} \
	} while(0)

#define insert(h, c, prnt) \
	do {\
		h->certain_cpos = 0; \
		if ((h->used+1) >= h->alloced) { \
			h->alloced += h->grow; \
			h->line = realloc(h->line, h->alloced); \
		} \
		if (h->cursor != h->used) {\
			trprintf(" mm %d %d %d\n", h->cursor+1, h->cursor, h->used-(h->cursor)); \
			memmove(h->line+h->cursor+1, h->line+h->cursor, h->used-(h->cursor)); \
		} \
		h->line[h->cursor] = c; \
		h->used++; \
		h->line[h->used] = '\0'; \
		h->cursor++; \
		if (prnt) {\
			char cb__ = c; \
			h->write(h, &cb__, 1); \
			if  (h->term.ccol == h->term.maxcol) {\
				h->write(h, "\n", 1); \
				h->term.ccol = 1; \
			} \
		} \
		if (h->cursor != h->used) {\
			vt100_csave(&(h->term)); \
			hrl_puts(h, h->line+h->cursor); \
			vt100_crestore(&(h->term)); \
		} \
		h->term.ccol++; \
		update_curspos(h); \
	} while(0)

static void store(hookreadline_t *h, const char *nw)
{
	h->used = strlen(nw);
	if ((h->used+1) >= h->alloced) {
		h->alloced = h->used+1;
		h->line = realloc(h->line, h->alloced);
	}
	memcpy(h->line, nw, h->used+1);
	h->cursor = h->used;
}


static void redraw(hookreadline_t *h, int content)
{
	int row, col, c;

	if (content) {
		trprintf("move redraw %d %d\n", h->srow, h->scol);
		vt100_move(&(h->term), h->srow, h->scol - strlen(h->prompt));
#warning TODO: make this more efficient
		hrl_puts(h, h->prompt);
		if (h->line != NULL)
			hrl_puts(h, h->line);
		h->write(h, " ", 1);
	}

	c = h->promptlen + h->cursor;
	row = c / h->term.maxcol;
	col = c % h->term.maxcol;
	if (row > h->max_row_offs) {
		h->max_row_offs = row;
		h->srow--;
		h->srow_pending_at = row + h->srow;
		h->certain_cpos = 0;
		trprintf("q2\n");
		query_curpos(h);
	}

/* TODO: this one is required on IRIX but fails on Linux */
/*	if (row > 0)
		col++;
*/
	trprintf("move redraw2 %d %d\n", row + h->srow, col+1);
	vt100_move(&(h->term), row + h->srow, col+1);

	h->term.crow = row + h->srow;
	h->term.ccol = col + 1;
}

static void del(hookreadline_t *h, int where, int move)
{
	int n;
	const char delchar = 8;

	memmove(h->line+where, h->line+where+1, h->used - where);
	h->used--;
	h->cursor += move;
	if (move < 0) {
		for(n = -move; n > 0; n--)
			h->write(h, &delchar, 1);
		if (h->term.ccol >= 1) {
			if (h->term.ccol <= 2)
				query_curpos(h);
			h->term.ccol--;
		}
	}
	hrl_puts(h, h->line+h->cursor);
	h->write(h, " ", 1);
	redraw(h, 0);
}

void hrl_calibrate(hookreadline_t *h)
{
	int ic, ir, c,r,last_col,last_row,chr;
	int found_row = 0;
	int found_col = 0;

	if (h->term_enter != NULL)
		h->term_enter(h);
	vt100_where(&(h->term));

	h->term.maxcol = h->term.maxrow = 0;
	last_row = last_col = 1;
	for(ic = -1, ir = -1;;) {
		chr = h->readc(h);
		switch(vt100_interpret(&(h->term), chr)) {
			case VT100_WHERE:
/*					trprintf("= curr=%d;%d max=%d;%d\n", h->term.crow, h->term.ccol, h->term.maxrow, h->term.maxcol);*/

					if (found_row) {
						if (h->term.maxcol <= last_col)
							found_col = 1;
					}

					if (h->term.maxrow <= last_row) {
						found_row = 1;
						r = 19;
					}

					if (found_col & found_row) {
						vt100_move(&(h->term), ir, ic);
						goto quit;
					}
				last_col = h->term.ccol;
				last_row = h->term.crow;
				if (ic < 0) {
					c = ic = h->term.ccol;
					r = ir = h->term.crow;
					h->term.maxrow = 0;
					r++;
				}
				else {
					if (!found_row)
						r++;
					else
						c++;
				}
/*				trprintf("MOVE %d;%d\n", r, c);*/
				vt100_move(&(h->term), r, c);
/*				trprintf("> %d;%d found: %d %d\n", r, c, found_row, found_col);*/
				vt100_where(&(h->term));
				break;
		}
	}
	quit:;
	if (h->term_leave != NULL)
		h->term_leave(h);
}

static int ctrl_chr(hookreadline_t *h, int c)
{
	if (h->ctrl_char != NULL)
		return h->ctrl_char(h, c);
	return 0;
}

char *hrl_readline(hookreadline_t *h, hrl_action_t action)
{
	int c;
	char *ret;
	const char *nw;

	if (h->suspended_row < 0) {
		if (h->term_enter != NULL)
			h->term_enter(h);
		if (h->prompt != NULL) {
			if ((!h->prompt_printed) || (action == HRL_REDRAW)) {
				h->promptlen = strlen(h->prompt);
				h->write(h, h->prompt, h->promptlen);
				h->prompt_printed = 1;
			}
		}
		else
			h->promptlen = 0;

		if (!h->queried_pos) {
			trprintf("q3\n");
			query_curpos(h);
			h->queried_pos = 1;
		}
		h->scol = -1;
		h->srow = -1;
		trprintf("ZERO used\n");
		if (!h->preloaded) {
			h->used = 0;
			h->cursor = 0;
			h->preloaded = 0;
		}
		else {
			h->write(h, h->line, h->used);
		}
		h->redraw = 0;
		h->max_row_offs = 0;
		h->srow_pending_at = -1;
		h->edited = 0;
		h->certain_cpos = 0;
	}
	else {
		trprintf("move initsus %d %d\n", h->suspended_row, h->suspended_col);
		if ((h->redraw) || (action == HRL_REDRAW)) {
			redraw(h, 1);
			h->redraw = 0;
		}
		else {
			vt100_move(&h->term, h->suspended_row, h->suspended_col);
			h->suspended_row = -1;
			h->suspended_col = -1;
		}
	}

	if (action == HRL_REDRAW)
		goto suspend;

	for(;;) {
		c = h->readc(h);
		if (c == HRL_NOMORE)
			goto suspend;
		if (c > 0) {
			int vt100c;
			vt100c = vt100_interpret(&(h->term), c);
			switch(vt100c) {
				case VT100_SEQ:
				case VT100_KEY_ESC:
					/* ignore these events/keys */
					break;
				case VT100_WHERE:
					h->queried_pos = 0;
					if (h->ignore_pos) {
						h->ignore_pos = 0;
						break;
					}
					h->certain_cpos = 1;
					trprintf("cursor ANSWER: r=%d c=%d curs=%d\n", h->term.crow, h->term.ccol, h->cursor);
					if (h->scol < 0) {
						h->scol = h->term.ccol;
						h->srow = h->term.crow;
					}
					if (h->srow_pending_at >= 0) {
						if (h->term.crow != h->srow_pending_at) {
							h->srow++;
							redraw(h, 0);
						}
						h->srow_pending_at = -1;
					}
					break;
				case '\n':
				case '\r':
					h->cursor = h->used;
					insert(h, '\0', 0);
					ret = h->line;
					goto out;
				case '\t':
					if (h->complete != NULL) {
						char *line;
						int cursor;
						line = h->line;
						cursor = h->cursor;
						if (h->complete(h, &line, &cursor)) {
							h->line = line;
							h->cursor = cursor;
							h->alloced = h->used = strlen(h->line);
							redraw(h, 1);
						}
					}
					break;
				case 4: /* ^D */
					ret = NULL;
					goto out;
				case 12: /* ^L */
					redraw(h, 1);
					break;
				case 8:
				case 127: /* backspace */
					if (h->cursor > 0) {
						del(h, h->cursor-1, -1);
					}
					break;
				case 126: /* del */
					if (h->cursor < h->used) {
						del(h, h->cursor, 0);
					}
					break;
				case VT100_KEY_UP:
					trprintf("hist up\n");
					nw = h->hist_up(h, h->edited ? h->line : NULL);
					if (nw != NULL) {
						h->certain_cpos = 0;
						store(h, nw);
						redraw(h, 1);
					}
					h->edited = 0;
					break;
				case VT100_KEY_DOWN:
					nw = h->hist_down(h);
					if (nw != NULL) {
						h->certain_cpos = 0;
						store(h, nw);
						redraw(h, 1);
					}
					h->edited = 0;
					break;
				case VT100_KEY_LEFT:
					h->cursor--;
					if (h->cursor >= 0) {
						h->term.ccol--;
						if (h->term.ccol < 1) {
							h->term.ccol = h->term.maxcol;
							h->term.crow--;
						}
					}
					else
						h->cursor = 0;
					h->certain_cpos = 0;
					redraw(h, 0);
					break;
				case VT100_KEY_RIGHT:
					h->cursor++;
					if (h->cursor >= h->used)
						h->cursor = h->used;
					h->certain_cpos = 0;
					redraw(h, 0);
					update_curspos(h);
					break;
/*				case '.': goto suspend;*/
				default:
					if (vt100c < 0) {
						ctrl_chr(h, vt100c);
					}
					else if (c < 32) {
						ctrl_chr(h, c);
					}
					else {
						trprintf("insert '%c' at %d/%d\n", c, h->cursor,h->used);
						insert(h, c, 1);
						h->edited++;
					}
			}
		}
	}

	out:;
	if (h->term_leave != NULL)
		h->term_leave(h);
	h->suspended_row = -1;
	h->suspended_col = -1;
	h->prompt_printed = 0;
	if (h->term_leave != NULL)
		h->term_leave(h);
	if (h->queried_pos)
		h->ignore_pos = 1;
	trprintf("out\n");
	return ret;

	suspend:;
	if (!h->certain_cpos) {
		trprintf("q4\n");
		query_curpos(h);
		h->queried_pos = 1;
	}
	if (h->scol > 0)  {
		trprintf("susp save: %d %d\n", h->term.crow, h->term.ccol);
		h->suspended_row = h->term.crow;
		h->suspended_col = h->term.ccol;
	}
	return NULL;
}

void hrl_stdio_enter(hookreadline_t *h)
{
	struct termios t;

	if (tcgetattr(0, &t) < 0)
		return;

	t.c_lflag &= ~ICANON;
	t.c_lflag &= ~ECHO;
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 10;

	tcsetattr(0, TCSANOW, &t);
}

void hrl_stdio_leave(hookreadline_t *h)
{
	struct termios t;

	if (tcgetattr(0, &t) < 0)
		return;

	t.c_lflag |= ICANON;
	t.c_lflag |= ECHO;

	tcsetattr(0, TCSANOW, &t);
}

typedef struct history_s history_t;

struct history_s {
	const char *line;
	history_t *next, *prev;
};

const char *hrl_hist_up(hookreadline_t *h, const char *current)
{
	history_t *hi;
	int len;

	if (h->historyp == NULL)
		h->historyp = h->history;
	else
		h->historyp = ((history_t *)(h->historyp))->next;

	if (h->historyp == NULL)
		return NULL;

	if (current != NULL) {
		len = strlen(current)+1;
		hi = malloc(sizeof(history_t));
		hi->line = malloc(len); /* strdup() is not portable */
		memcpy((char *)hi->line, current, len);
		hi->next = h->history;
		hi->prev = NULL;
		hi->next->prev = h->history = h;
	}
	return ((history_t *)(h->historyp))->line;
}

const char *hrl_hist_down(hookreadline_t *h)
{
	if (h->historyp == NULL)
		return NULL;
	h->historyp = ((history_t *)(h->historyp))->prev;
	if (h->historyp == NULL)
		return NULL;
	return ((history_t *)(h->historyp))->line;
}

int hrl_stdio_readc(hookreadline_t *h)
{
	unsigned char c;
	int len;
	len = read(0, &c, 1);
	if (len <= 0)
		return -1;
	return c;
}

int hrl_stdio_write(hookreadline_t *h, const char *s, int len)
{
	int acc;
	acc = 0;
	while(len > 0) {
		int l;
		l = write(1, s, len);
		if (l <= 0)
			return -1;
		len -= l;
		s += l;
		acc += l;
	}
	return acc;
}

hookreadline_t *hrl_default(hookreadline_t *h)
{
	if (h == NULL) {
		h = malloc(sizeof(hookreadline_t));
		h->dynamic = 1;
	}
	else
		h->dynamic = 0;

	h->readc = hrl_stdio_readc;
	h->write = hrl_stdio_write;
	h->term_enter = hrl_stdio_enter;
	h->term_leave = hrl_stdio_leave;
	h->ctrl_char = NULL;
	h->line = NULL;
	h->prompt = NULL;
	h->hist_up   = hrl_hist_up;
	h->hist_down = hrl_hist_down;
	h->complete  = NULL;
	h->suspended_row = -1;
	h->queried_pos = 0;
	h->ignore_pos = 0;
	h->prompt_printed = 0;

	h->history = h->historyp = h->user_data = NULL;

	h->grow = 128;

	h->used = h->alloced = 0;
	memset(&(h->term), 0, sizeof(h->term));
	vt100_init(&(h->term), (int (*)(void *, const char *, int))h->write, h);
	return h;
}


void hrl_destroy(hookreadline_t *h)
{
	if (h->line != NULL) {
		free(h->line);
		h->line = NULL;
	}
	h->used = h->alloced = 0;
	if (h->prompt != NULL) {
		free(h->prompt);
		h->prompt = NULL;
	}
}

void hrl_set_line(hookreadline_t *h, const char *line)
{
	h->used = strlen(line);
	if (h->alloced < h->used+1) {
		free(h->line);
		h->alloced = h->used + h->grow;
		h->line = malloc(h->alloced);
	}
	memcpy(h->line, line, h->used+1);
	h->cursor = h->used;
	h->redraw = 1;
	h->preloaded = 1;
}

void hrl_set_prompt(hookreadline_t *h, const char *prompt)
{
	int len = strlen(prompt)+1;
	if (h->prompt != NULL)
		free(h->prompt);
	h->prompt = malloc(len);
	memcpy(h->prompt, prompt, len);
}
