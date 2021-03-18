/*
    scconfig - simple c menus - windowing
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
#include <string.h>
#include "scmenu.h"
#include "vt100.h"

#define TERM (&ctx->hrl.term)

/* enough spaces to fill in a row on the screen */
static const char *spaces = "                                                                                                                                                                                                                                                                ";
static const char *dashes = "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";

int scm_init(scm_ctx_t *ctx)
{
	hrl_default(&ctx->hrl);
	hrl_calibrate(&ctx->hrl);
	vt100_clear(&ctx->hrl.term);
	return 0;
}

static int draw_box(scm_ctx_t *ctx, scm_coord_t x1, scm_coord_t y1, scm_coord_t w, scm_coord_t h, int top_only)
{
	int n;
	char frame_line[257];
	char cont_line[257];

	memset(&frame_line[1], '-', w-2);
	frame_line[0] = '+';
	frame_line[w-1] = '+';

	vt100_move(TERM, y1, x1);
	vt100_write(TERM, frame_line, w);
	if (top_only)
		return 0;
	vt100_move(TERM, y1+h, x1);
	vt100_write(TERM, frame_line, w);

	memset(&cont_line[1], ' ', w-2);
	cont_line[0] = '|';
	cont_line[w-1] = '|';

	for(n = y1+1; n < y1+h; n++) {
		vt100_move(TERM, n, x1);
		vt100_write(TERM, cont_line, w);
	}

	return 0;
}

static int clear_box(scm_ctx_t *ctx, scm_coord_t x1, scm_coord_t y1, scm_coord_t w, scm_coord_t h)
{
	int n;

	for(n = y1; n <= y1+h; n++) {
		vt100_move(TERM, n, x1);
		vt100_write(TERM, spaces, w);
	}

	return 0;
}

static int window_draw(scm_ctx_t *ctx, scm_win_t *win, int title_only)
{
	vt100_mode_normal(TERM);
	draw_box(ctx, win->x1, win->y1, win->w, win->h, win->top_only | title_only);
	if (win->title != NULL) {
		int len, x1, sl = 0;
		char *suffix;

		len = strlen(win->title);
		if (len > win->w - 4) {
			len = win->w - 12;
			if (len < 0)
				len = 0;
			suffix=" ...";
			sl = strlen(suffix);
		}
		x1 = win->x1 + (win->w - (len+4+sl)) / 2;

		vt100_move(TERM, win->y1, x1);
		vt100_write(TERM, "{", 1);
		if (win->active)
			vt100_mode_inverse(TERM);
		else
			vt100_mode_normal(TERM);

		vt100_write(TERM, " ", 1);
		vt100_write(TERM, win->title, len);
		if (sl > 0)
			vt100_write(TERM, suffix, sl);
		vt100_write(TERM, " ", 1);
		if (win->active)
			vt100_mode_normal(TERM);
		vt100_write(TERM, "}", 1);
	}
	return 0;
}

static int window_clear(scm_ctx_t *ctx, scm_win_t *win)
{
	vt100_mode_normal(TERM);
	clear_box(ctx, win->x1, win->y1, win->w, win->h);
	return 0;
}

static void win_adjust(scm_ctx_t *ctx, scm_win_t *win, int cwidth, int cheight)
{
	if (win->title != NULL) {
		int tlen;
		tlen = strlen(win->title)+8;
		if (tlen > cwidth)
			cwidth = tlen;
	}

	if (win->cfg_w == 0) {
		win->w = cwidth+2;
		if (win->w > TERM->maxcol)
			win->w = TERM->maxcol;
	}
	else
		win->w = win->cfg_w;

	if (win->cfg_h == 0) {
		win->h = cheight+2;
		if (win->h > TERM->maxrow-1)
			win->h = TERM->maxrow-1;
	}
	else
		win->h = win->cfg_h;

	if (win->cfg_x1 == 0)
		win->x1 = (TERM->maxcol - win->w) / 2;
	else
		win->x1 = win->cfg_x1;

	if (win->cfg_y1 == 0)
		win->y1 = (TERM->maxrow - win->h) / 2;
	else
		win->y1 = win->cfg_y1;

	if (win->x1 < 1)
		win->x1 = 1;
	if (win->y1 < 1)
		win->y1 = 1;
}

int scm_menu_autowin(scm_ctx_t *ctx, const char *title, scm_menu_entry_t *entries)
{
	scm_win_t w;
	scm_menu_t m;

	memset(&w, 0, sizeof(w));
	w.title = title;

	memset(&m, 0, sizeof(m));
	m.entries = entries;
	m.num_entries = -1;
	m.cursor = 0;

	scm_menu(ctx, &m, &w, NULL);
}



#include "scmenu_menu.c"
#include "scmenu_rl.c"
#include "scmenu_text.c"
