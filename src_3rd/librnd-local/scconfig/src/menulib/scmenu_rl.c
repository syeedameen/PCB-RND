/*
    scconfig - simple c menus - readline window
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

char *scm_readline(scm_ctx_t *ctx, const char *title, const char *prompt, const char *orig)
{
	scm_win_t w;
	char *res;

	memset(&w, 0, sizeof(w));
	w.title = title;
	w.h = 5;
	w.w = TERM->maxcol;
	w.x1 = 1;
	w.y1 = TERM->maxrow - w.h;
	w.top_only = 1;
	w.active = 1;

	window_draw(ctx, &w, 0);
	vt100_move(TERM, w.y1+1, 1);

	hrl_set_prompt(&ctx->hrl, prompt);
	hrl_set_line(&ctx->hrl, orig);
	res = hrl_readline(&ctx->hrl, HRL_READLINE);

	window_clear(ctx, &w);

	return res;
}
