/*
    scconfig - simple c menus - text window and popup
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

static char *button_text[]   = {"yes", "no", "ok", "cancel", "quit"};
static int button_text_len[] = {3,     2,    2,    6,        4};

static void text_scroll_valid(scm_text_t *text, scm_win_t *win)
{
	if (text->scroll < 0)
		text->scroll = 0;

	if (text->scroll >= text->num_rows)
		text->scroll = text->num_rows-1;
}

static void text_draw_buttons(scm_ctx_t *ctx, scm_text_t *text, scm_win_t *win)
{
	int n, c, cx, x;

	cx = x = win->x1+text->button_start;
	vt100_move(TERM, win->y1+win->h-1, x);
	for(n = 0, c = 0; n < SCM_BUTTON_BITS; n++) {
		if (text->buttons & (1 << n)) {
			vt100_write(TERM, " <", 2);
			if (text->bcursor == c) {
				vt100_mode_highlight(TERM);
				cx = x + 2 + button_text_len[n]/2;
			}
			vt100_write(TERM, button_text[n], button_text_len[n]);
			if (text->bcursor == c)
				vt100_mode_normal(TERM);
			vt100_write(TERM, "> ", 2);
			c++;
			x += 4 + button_text_len[n];
		}
	}
	vt100_move(TERM, win->y1+win->h-1, cx);
}

static void text_draw(scm_ctx_t *ctx, scm_text_t *text, scm_win_t *win)
{
	int n;
	char **r;

	text_scroll_valid(text, win);
	window_draw(ctx, win, 0);
	vt100_mode_normal(TERM);
	for(n = 0, r = text->row + text->scroll; (n < win->h - 4) && (r < text->row + text->num_rows); n++, r++) {
		vt100_move(TERM, win->y1+n+2, win->x1+1);
		vt100_write(TERM, *r, strlen(*r));
	}
	text_draw_buttons(ctx, text, win);
}

static void text_moved(scm_ctx_t *ctx, scm_text_t *text, scm_win_t *win)
{
	text_scroll_valid(text, win);

	if (text->prev_scroll != text->scroll)
		text_draw(ctx, text, win);
}

void scm_text_move(scm_ctx_t *ctx, scm_text_t *text, scm_win_t *win, int delta)
{
	text->prev_scroll = text->scroll;
	text->scroll += delta;
	text_moved(ctx, text, win);
}

void scm_text_moveto(scm_ctx_t *ctx, scm_text_t *text, scm_win_t *win, int pos)
{
	text->prev_scroll = text->scroll;
	text->scroll = pos;
	text_moved(ctx, text, win);
}

int scm_text_run(scm_ctx_t *ctx, scm_text_t *text, scm_win_t *win)
{
	int res = -2;

	ctx->hrl.term_enter(&ctx->hrl);
	while(res == -2) {
		int c;
		c = ctx->hrl.readc(&ctx->hrl);
		c = vt100_interpret(TERM, c);
		switch(c) {
			case VT100_KEY_UP:
			case 'k':
				scm_text_move(ctx, text, win, -1);
				break;
			case VT100_KEY_DOWN:
			case 'j':
				scm_text_move(ctx, text, win, +1);
				break;
			case VT100_KEY_PGUP:
			case 'u':
				scm_text_move(ctx, text, win, -(win->h-3));
				break;
			case VT100_KEY_PGDN:
			case 'i':
				scm_text_move(ctx, text, win, +(win->h-3));
				break;
			case VT100_KEY_HOME:
				scm_text_moveto(ctx, text, win, 0);
				break;
			case VT100_KEY_END:
				scm_text_moveto(ctx, text, win, text->num_rows);
				break;
			case '\t':
				text->bcursor++;
				if (text->bcursor >= text->num_buttons)
					text->bcursor = 0;
				text_draw_buttons(ctx, text, win);
				break;
			case VT100_KEY_RIGHT:
				text->bcursor++;
				if (text->bcursor >= text->num_buttons)
					text->bcursor = text->num_buttons-1;
				text_draw_buttons(ctx, text, win);
				break;
			case VT100_KEY_LEFT:
				text->bcursor--;
				if (text->bcursor < 0)
					text->bcursor = 0;
				text_draw_buttons(ctx, text, win);
				break;
			case '\r':
			case '\n':
				{
					int n, c;
					for(n = 0, c = 0; n < SCM_BUTTON_BITS; n++) {
						if (text->buttons & (1 << n)) {
							if (text->bcursor == c)
								res = (1 << n);
							c++;
						}
					}
				}
				break;
			case 27:
				res = -1;
				break;
		}
	}
	ctx->hrl.term_leave(&ctx->hrl);

	return res;
}



scm_button_bits_t scm_text(scm_ctx_t *ctx, scm_text_t *text, scm_win_t *win)
{
	int bwidth, cwidth, r;
	scm_button_bits_t res;

	text->num_buttons = 0;
	if (text->buttons == 0)
		text->buttons = SCM_OK;

	bwidth = 0;
	for(r = 0; r < SCM_BUTTON_BITS; r++) {
		if (text->buttons & (1 << r)) {
			bwidth += 4 + button_text_len[r];
			text->num_buttons++;
		}
	}

	cwidth = bwidth;
	for(r = 0; r < text->num_rows; r++) {
		int l = strlen(text->row[r]);
		if (l > cwidth)
			cwidth = l;
	}

	win->active = 1;
	win_adjust(ctx, win, cwidth, text->num_rows + 2);
	text->button_start = (win->w - bwidth) / 2;
	text_draw(ctx, text, win);

	res = scm_text_run(ctx, text, win);

	window_clear(ctx, win);

	return res;
}

scm_button_bits_t scm_popup(scm_ctx_t *ctx, const char *title, const char *text_, scm_button_bits_t buttons)
{
	int tlen = strlen(text_), r;
	char *text, *s, *start;
	scm_win_t w;
	scm_text_t t;
	scm_button_bits_t res;

	text = malloc(tlen+1);
	memcpy(text, text_, tlen+1);

	memset(&t, 0, sizeof(t));

	t.buttons = buttons;
	t.num_rows = 1;
	for(s = text; *s != '\0'; s++) {
		if (*s == '\n')
			t.num_rows++;
		if (*s == '\r')
			*s = ' ';
	}

	t.row = malloc(sizeof(char *) * t.num_rows);
	for(start = s = text, r=0; *s != '\0'; s++) {
		if (*s == '\n') {
			*s = '\0';
			if (start != s) {
				t.row[r] = start;
				r++;
			}
			start = s+1;
		}
	}
	t.row[r] = start;

	memset(&w, 0, sizeof(w));
	w.title = title;

	res = scm_text(ctx, &t, &w);
	free(text);
	return res;
}

