/*
    scconfig - vt100 controls
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


#ifndef VT100_H
#define VT100_H

#include <stdio.h>

typedef enum vt100_keycodes_e {
	VT100_SEQ = 256, /* in sequence, need more characters */
	VT100_WHERE,
	VT100_KEY_UP,
	VT100_KEY_DOWN,
	VT100_KEY_LEFT,
	VT100_KEY_RIGHT,
	VT100_KEY_PGUP,
	VT100_KEY_PGDN,
	VT100_KEY_HOME,
	VT100_KEY_END,
	VT100_KEY_ESC,
} vt100_keycodes_t;


typedef enum vt100_istate_s {
	ST_FREE, /* not in sequence */
	ST_ESC,  /* in sequence, read esc */
	ST_SEQ
} vt100_istate_t;

typedef struct vt100_state_s {
	int (*write)(void *ud, const char *s, int len);
	vt100_istate_t state;
	int crow, ccol;
	int maxrow, maxcol;
	char seq[128];
	int seqi;
	void *user_data;
} vt100_state_t;

#define vt100_puts(state, s)        state->write(state->user_data, s, strlen(s))
#define vt100_write(state, s, len)  state->write(state->user_data, s, len)

vt100_state_t *vt100_init(vt100_state_t *state, int (*write)(void *handle, const char *s, int len), void *user_data);
int vt100_interpret(vt100_state_t *state, char c);

void vt100_where(vt100_state_t *state);
void vt100_move(vt100_state_t *state, int row, int col);
void vt100_csave(vt100_state_t *state);
void vt100_crestore(vt100_state_t *state);

void vt100_clear(vt100_state_t *state);

void vt100_mode_normal(vt100_state_t *state);
void vt100_mode_inverse(vt100_state_t *state);
void vt100_mode_highlight(vt100_state_t *state);
void vt100_mode_blink(vt100_state_t *state);
void vt100_mode_underline(vt100_state_t *state);

#define vt100_keycode_alt(chr)  ((int)chr * -1)
#define vt100_keycode_ctrl(chr)  (chr - 'a' + 1)

#endif
