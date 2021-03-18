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


#include "vt100.h"

typedef struct hookreadline_s hookreadline_t;

struct hookreadline_s {
	char *prompt;     /* must be malloced, will be free'd */
	int dynamic;      /* free pointer on destroy */

	/* I/O hooks */
	int (*readc)(hookreadline_t *h);
	int (*write)(hookreadline_t *h, const char *s, int len);
	void (*term_enter)(hookreadline_t *h); /* called at enter to turn off line buffering */
	void (*term_leave)(hookreadline_t *h); /* called at enter to turn off line buffering */

	/* event hooks */
	const char *(*hist_up)(hookreadline_t *h, const char *current); /* user pressed up arrow; current is line being edited or NULL for subsequent up arrows without editing; return next history line or NULL */
	const char *(*hist_down)(hookreadline_t *h);                    /* user pressed up arrow */
	int (*complete)(hookreadline_t *h, char **line, int *cursor);   /* callback for tab-completion; returns non-zero on change in line and cursor (line may be realloc'd) */
	int (*ctrl_char)(hookreadline_t *h, int c);                     /* control char received; alt+char is -ascii; ctrl+alpha is 1..27 */


	/* for the caller to store data */
	void *history, *historyp;
	void *user_data;

	/* advanced configuration */
	int grow;           /* when line needs to grow, how many new bytes to allocate */
	int redraw;         /* full redraw in the next call */

	/* internal */
	char *line;          /* line being edited, terminated with \0 */
	int used, alloced;   /* line length and allocation info */
	int preloaded;       /* 1 if line should not be cleared when entering htl_readline() */
	int cursor;          /* cursor index */
	vt100_state_t term;  /* terminal info */
	int srow, scol;      /* start row, start col */
	int promptlen;       /* length of prompt cache */
	int edited;          /* whether user edited current line already (used for hist_up) */
	int certain_cpos;    /* are we certain about cursor pos? */

	/* internal: for scroll-hack */
	int max_row_offs;    /* max row offset seen while moving cursor */
	int srow_pending_at; /* was at physical row when last hacked start row */
	int prompt_printed;  /* the prompt has been printed already - matters in non-blocking mode */
	int queried_pos;     /* guarantee there's only one outstanding cursor pos query at a time to avoid falling out of sync */
	int ignore_pos;

	/* if >=0: there is an edit in progress that was suspended in the last call; these are the previous cursor coords */
	int suspended_row, suspended_col;
};

typedef enum hrl_action_e {
	HRL_READLINE = 0,                 /* normal readline in blocking or non-blocking mode */
	HRL_REDRAW,                       /* redraw and return */
} hrl_action_t;

hookreadline_t *hrl_default(hookreadline_t *h);
void hrl_destroy(hookreadline_t *h);
char *hrl_readline(hookreadline_t *h, hrl_action_t action);

void hrl_set_line(hookreadline_t *h, const char *line);
void hrl_set_prompt(hookreadline_t *h, const char *prompt);

/* measure terminal dimensions */
void hrl_calibrate(hookreadline_t *h);

/* default hooks */
void hrl_stdio_enter(hookreadline_t *h);
void hrl_stdio_leave(hookreadline_t *h);

/* readc() should return HRL_NOMORE in non-blocking mode if it would block */
#define HRL_NOMORE (-2)

#define hrl_suspended(h)  ((h)->suspended_row >= 0)
