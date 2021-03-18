/*
    scconfig - simple c menus - menu window
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

static void menu_scroll_valid(scm_menu_t *menu, scm_win_t *win)
{
	int emin, emax;

	if (menu->cursor < 0)
		menu->cursor = 0;

	if (menu->cursor >= menu->num_entries)
		menu->cursor = menu->num_entries-1;

	if (menu->scroll >= menu->num_entries - (win->h-3))
		menu->scroll = menu->num_entries - win->h;
	if (menu->scroll < 0)
		menu->scroll = 0;

	emin = menu->scroll;
	emax = menu->scroll + (win->h - 3);
	if (menu->cursor < emin)
		menu->scroll -= emin - menu->cursor;
	if (menu->cursor > emax)
		menu->scroll += menu->cursor - emax;

}


static int menu_entry_draw_key(scm_ctx_t *ctx, scm_menu_t *menu, scm_menu_entry_t *entry, int keylen, int silent)
{
	if (keylen > menu->kwidth)
		keylen = menu->kwidth;
	if (!silent)
		vt100_write(TERM, entry->key, keylen);
	if (keylen < menu->kwidth) {
		if (!silent)
			vt100_write(TERM, spaces, menu->kwidth - keylen);
		return menu->kwidth;
	}
	return keylen;
}

static int menu_entry_draw_(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win, scm_menu_entry_t *entry, int silent, int *keylen, int ret_padded_len)
{
	int *klen, vlen, klen_tmp, plen;
	if (keylen == NULL)
		klen = &klen_tmp;
	else
		klen = keylen;

	if ((menu->is_select) && (!silent)) {
		if (entry->flags & SCM_RADIO) {
			if (entry->flags & SCM_CHECKED)
				vt100_write(TERM, "(*) ", 4);
			else
				vt100_write(TERM, "( ) ", 4);
		}
		else if (entry->flags & SCM_CHECKBOX) {
			if (entry->flags & SCM_CHECKED)
				vt100_write(TERM, "[*] ", 4);
			else
				vt100_write(TERM, "[ ] ", 4);
		}
		else {
			switch(entry->type) {
				case SCM_TERMINATOR:
				case SCM_EMPTY:
				case SCM_SEPARATOR:
					break;
				case SCM_KEY_ONLY:
				case SCM_KEY_VALUE:
				case SCM_KEY_VALUE_EDIT:
				case SCM_COMBO:
				case SCM_SUBMENU:
				case SCM_SUBMENU_CB:
					vt100_write(TERM, "    ", 4);
					break;
			}
		}
	}

	switch(entry->type) {
		case SCM_TERMINATOR:
		case SCM_EMPTY:
			*klen = 0;
			return 0;
		case SCM_SEPARATOR:
			if (!silent)
				vt100_write(TERM, dashes, win->w-2);
			return 0;
		case SCM_KEY_ONLY:
			*klen = strlen(entry->key);
			plen = menu_entry_draw_key(ctx, menu, entry, *klen, silent);
			if (ret_padded_len)
				*klen = plen;
			return *klen;
		case SCM_KEY_VALUE:
		case SCM_SUBMENU:
		case SCM_SUBMENU_CB:
			*klen = strlen(entry->key);
			vlen = strlen(entry->value);
			plen = menu_entry_draw_key(ctx, menu, entry, *klen, silent);
			if (!silent) {
				vt100_write(TERM, "  ", 2);
				vt100_write(TERM, entry->value, vlen);
			}
			if (ret_padded_len)
				*klen = plen;
			return *klen+vlen+2;
		case SCM_KEY_VALUE_EDIT:
		case SCM_COMBO:
			*klen = strlen(entry->key);
			vlen = strlen(entry->value);
			plen = menu_entry_draw_key(ctx, menu, entry, *klen, silent);
			if (!silent) {
				vt100_write(TERM, " [", 2);
				vt100_write(TERM, entry->value, vlen);
				vt100_write(TERM, "]", 1);
			}
			if (ret_padded_len)
				*klen = plen;
			return *klen+vlen+2+1;
	}
	return 0;
}

static void menu_set_cursor(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win)
{
	vt100_move(TERM, menu->cursor - menu->scroll + win->y1+2, win->x1+1+(!!menu->is_select));
}

static void menu_entry_draw(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win, int winrow, scm_menu_entry_t *e)
{
	vt100_move(TERM, win->y1+winrow+2, win->x1+1);
	menu_entry_draw_(ctx, menu, win, e, 0, NULL, 0);
}

static void menu_draw(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win)
{
	int n;
	scm_menu_entry_t *e;

	menu_scroll_valid(menu, win);
	window_draw(ctx, win, 0);
	vt100_mode_normal(TERM);
	for(n = 0, e = menu->entries + menu->scroll; n < win->h-2 && e->type != SCM_TERMINATOR; n++, e++) {
		if (menu->scroll+n == menu->cursor)
			vt100_mode_highlight(TERM);
		menu_entry_draw(ctx, menu, win, n, e);
		if (menu->scroll+n == menu->cursor)
			vt100_mode_normal(TERM);
	}

	menu_set_cursor(ctx, menu, win);
}

static void menu_moved(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win)
{
	int old_scroll = menu->scroll;

	menu_scroll_valid(menu, win);

	if (old_scroll == menu->scroll) {
		vt100_mode_normal(TERM);
		menu_entry_draw(ctx, menu, win, menu->prev_cursor - menu->scroll, &menu->entries[menu->prev_cursor]);
		vt100_mode_highlight(TERM);
		menu_entry_draw(ctx, menu, win, menu->cursor - menu->scroll, &menu->entries[menu->cursor]);
		vt100_mode_normal(TERM);
	}
	else
		menu_draw(ctx, menu, win);


	menu_set_cursor(ctx, menu, win);
}

void scm_menu_move(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win, int delta)
{
	menu->prev_cursor = menu->cursor;
	menu->cursor += delta;
	menu_moved(ctx, menu, win);
}

void scm_menu_moveto(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win, int pos)
{
	menu->prev_cursor = menu->cursor;
	menu->cursor = pos;
	menu_moved(ctx, menu, win);
}

static void scm_menu_skip(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win, int delta)
{
	menu->prev_cursor = menu->cursor;
	while((menu->entries[menu->cursor].type == SCM_SEPARATOR) || (menu->entries[menu->cursor].type == SCM_EMPTY)) {
		menu->cursor += delta;
		if (menu->cursor < 0) {
			menu->cursor = 0;
			break;
		}
		if (menu->cursor >= menu->num_entries) {
			menu->cursor = menu->num_entries;
			break;
		}
	}
	menu_moved(ctx, menu, win);
}

static void menu_toggle_current(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win)
{
	scm_menu_entry_t *e;
	int redraw = 0;

	e = menu->entries + menu->cursor;
	if (e->flags & SCM_RADIO) {
		if (menu->radio_cursor >= 0) {
			menu->entries[menu->radio_cursor].flags &= ~SCM_CHECKED;
			redraw = 1;
		}
		e->flags |= SCM_CHECKED;
		menu->radio_cursor = menu->cursor;
	}
	else if (e->flags & SCM_CHECKBOX)
		e->flags ^= SCM_CHECKED;

	if (!redraw) {
		vt100_mode_highlight(TERM);
		menu_entry_draw(ctx, menu, win, menu->cursor - menu->scroll, &menu->entries[menu->cursor]);
		vt100_mode_normal(TERM);
		menu_set_cursor(ctx, menu, win);
	}
	else
		menu_draw(ctx, menu, win);
}

static int combo_set(scm_ctx_t *ctx, scm_menu_t *menu, const char *value)
{
	int n, c;
	c = 0;
	for(n = 0; menu->entries[n].type != SCM_TERMINATOR; n++) {
		if (strcmp(value, menu->entries[n].key) == 0) {
			menu->entries[n].flags |= SCM_CHECKED;
			c = n;
		}
		else
			menu->entries[n].flags &= ~SCM_CHECKED;
	}
	return c;
}

int scm_menu_auto_run(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win, int item_idx)
{
	scm_menu_entry_t *e = &menu->entries[item_idx];
	switch(e->type) {
		case SCM_TERMINATOR:
		case SCM_EMPTY:
		case SCM_SEPARATOR:
			/* do nothing: */
			return -2;
		case SCM_KEY_ONLY:
		case SCM_KEY_VALUE:
			/* return the selection */
			break;
		case SCM_KEY_VALUE_EDIT:
			/* run a readline on value */
			{
				char *str;
				str = scm_readline(ctx, e->key, "", e->value);
				if (str != NULL)
					e->value = str;
			}
			return -2;
		case SCM_COMBO:
			{
				scm_win_t cwin;
				scm_menu_t cmenu;
				int res;

				memset(&cwin, 0, sizeof(cwin));
				cwin.title = e->key;

				memset(&cmenu, 0, sizeof(cmenu));
				cmenu.entries = (scm_menu_entry_t *)e->auto_data;
				cmenu.num_entries = -1;
				cmenu.cursor = combo_set(ctx, &cmenu, e->value);

				res = scm_menu(ctx, &cmenu, &cwin, NULL);
				if (res >= 0)
					e->value = cmenu.entries[res].key;
			}
			return -2;
		case SCM_SUBMENU_CB:
			{
				scm_menu_enter_t cb = (scm_menu_enter_t *)e->auto_data;
				return cb(ctx, menu, win, item_idx);
			}
		case SCM_SUBMENU:
			scm_menu_autowin(ctx,  e->key, e->auto_data);
			return -2;
	}
	return item_idx;
}


int scm_menu_run(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win, scm_menu_enter_t menu_enter)
{
	int res = -2;

	if (ctx->hrl.term_enter != NULL)
		ctx->hrl.term_enter(&ctx->hrl);

	while(res == -2) {
		int c;
		c = ctx->hrl.readc(&ctx->hrl);
		c = vt100_interpret(TERM, c);
		switch(c) {
			case VT100_KEY_UP:
			case 'k':
				scm_menu_move(ctx, menu, win, -1);
				scm_menu_skip(ctx, menu, win, -1);
				break;
			case VT100_KEY_DOWN:
			case 'j':
				scm_menu_move(ctx, menu, win, +1);
				scm_menu_skip(ctx, menu, win, +1);
				break;
			case VT100_KEY_PGUP:
			case 'u':
				scm_menu_move(ctx, menu, win, -(win->h-3));
				scm_menu_skip(ctx, menu, win, -1);
				break;
			case VT100_KEY_PGDN:
			case 'i':
				scm_menu_move(ctx, menu, win, +(win->h-3));
				scm_menu_skip(ctx, menu, win, +1);
				break;
			case VT100_KEY_HOME:
				scm_menu_moveto(ctx, menu, win, 0);
				scm_menu_skip(ctx, menu, win, +1);
				break;
			case VT100_KEY_END:
				scm_menu_moveto(ctx, menu, win, menu->num_entries);
				scm_menu_skip(ctx, menu, win, -1);
				break;
			case ' ':
				menu_toggle_current(ctx, menu, win);
				break;
			case '\r':
			case '\n':
				if ((menu_enter != NULL) || (menu->entries[menu->cursor].flags & SCM_AUTO_RUN)) {
					win->active = 0;
					window_draw(ctx, win, 1);
					if (ctx->hrl.term_leave != NULL)
						ctx->hrl.term_leave(&ctx->hrl);
					if (menu_enter != NULL)
						res = menu_enter(ctx, menu, win, menu->cursor);
					if (menu->entries[menu->cursor].flags & SCM_AUTO_RUN)
						res = scm_menu_auto_run(ctx, menu, win, menu->cursor);
					if (res != -2)
						return res;
					if (ctx->hrl.term_enter != NULL)
						ctx->hrl.term_enter(&ctx->hrl);
					win->active = 1;
					menu_draw(ctx, menu, win);
				}
				else {
					if (menu->has_radio != -1)
						res = menu->radio_cursor;
					else
						res = menu->cursor;
				}
				break;
			case 27:
				res = -1;
				break;
		}
	}
	if (ctx->hrl.term_leave != NULL)
		ctx->hrl.term_leave(&ctx->hrl);

	return res;
}

int scm_menu(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win, scm_menu_enter_t menu_enter)
{
	scm_win_t autowin;
	scm_menu_entry_t *e;
	int cwidth, res, c;

	if (win == NULL) {
		win = &autowin;
		memset(&autowin, 0, sizeof(autowin));
	}

	menu->num_entries = 0;
	menu->kwidth = 0;
	menu->is_select = 0;
	menu->radio_cursor = -1;
	menu->has_radio = 0;
	for(e = menu->entries, c = 0; e->type != SCM_TERMINATOR; e++, c++) {
		int kw;
		menu_entry_draw_(ctx, menu, win, e, 1, &kw, 0);
		if (kw > menu->kwidth)
			menu->kwidth = kw;
		menu->num_entries++;
		if (e->flags & (SCM_CHECKBOX | SCM_RADIO))
			menu->is_select = 1;

		/* keep only the first radio selection and put the radio cursor and cursor there */
		if (e->flags & SCM_RADIO) {
			menu->has_radio = 1;
			if (e->flags & SCM_CHECKED) {
				if (menu->radio_cursor == -1) {
					menu->radio_cursor = c;
					menu->cursor = c;
				}
				else
					e->flags ^= SCM_CHECKED;
			}
		}
	}

	cwidth = 0;
	for(e = menu->entries; e->type != SCM_TERMINATOR; e++) {
		int w;
		w = menu_entry_draw_(ctx, menu, win, e, 1, NULL, 1);
		if (w > cwidth)
			cwidth = w;
	}

	if (menu->is_select)
		cwidth += 4;

	win->active = 1;
	win_adjust(ctx, win, cwidth, menu->num_entries);
	menu_draw(ctx, menu, win);

	res = scm_menu_run(ctx, menu, win, menu_enter);

	if (menu_enter != NULL) {
		win->active = 0;
		menu_draw(ctx, menu, win);
	}
	else
		window_clear(ctx, win);

	return res;
}
