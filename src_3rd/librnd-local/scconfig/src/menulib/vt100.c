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


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vt100.h"

#define ESC "\x1b"

void vt100_where(vt100_state_t *state)
{
	vt100_puts(state, ESC "[6n");
}

void vt100_move(vt100_state_t *state, int row, int col)
{
	char s[128];
	int len;
	len = sprintf(s, ESC "[%d;%df", row, col);
	vt100_write(state, s, len);
}

void vt100_csave(vt100_state_t *state)
{
	vt100_puts(state, ESC "[s");
}

void vt100_crestore(vt100_state_t *state)
{
	vt100_puts(state, ESC "[u");
}

void vt100_clear(vt100_state_t *state)
{
	vt100_puts(state, ESC "c");
}

void vt100_mode_normal(vt100_state_t *state)
{
	vt100_puts(state, ESC "[m");
}

void vt100_mode_inverse(vt100_state_t *state)
{
	vt100_puts(state, ESC "[7m");
}

void vt100_mode_highlight(vt100_state_t *state)
{
	vt100_puts(state, ESC "[1m");
}

void vt100_mode_blink(vt100_state_t *state)
{
	vt100_puts(state, ESC "[5m");
}

void vt100_mode_underline(vt100_state_t *state)
{
	vt100_puts(state, ESC "[4m");
}

static int sequence(vt100_state_t *state, char term)
{
	char *end, *next;
	int i1, i2;
	switch(term) {
		case 'R':
			next = strchr(state->seq, ';');
			if (next == NULL)
				return VT100_SEQ;
			*next = '\0';
			next++;
			i1 = strtol(state->seq, &end, 10);
			if (*end != '\0')
				return VT100_SEQ;
			i2 = strtol(next, &end, 10);
			if (*end != '\0')
				return VT100_SEQ;
			state->crow  = i1;
			state->ccol = i2;
			if (i1 > state->maxrow)
				state->maxrow = i1;
			if (i2 > state->maxcol)
				state->maxcol = i2;
			return VT100_WHERE;
		case 'A': return VT100_KEY_UP;
		case 'B': return VT100_KEY_DOWN;
		case 'C': return VT100_KEY_RIGHT;
		case 'D': return VT100_KEY_LEFT;
		case '~':
			switch(state->seq[0]) {
				case '1': return VT100_KEY_HOME;
				case '4': return VT100_KEY_END;
				case '5': return VT100_KEY_PGUP;
				case '6': return VT100_KEY_PGDN;
			}
			break;
	}
	return VT100_SEQ;
}

void d1(){}

int vt100_interpret(vt100_state_t *state, char c_)
{
	int c = c_;
	switch(state->state) {
		case ST_FREE:
			switch(c) {
				case 27:
					state->state = ST_ESC;
					return VT100_SEQ;
				case '\r':
					return '\n';
				default:
					return c;
			}
			break;
		case ST_ESC:
			switch(c) {
				case '[':
					state->state = ST_SEQ;
					state->seqi = 0;
					return VT100_SEQ;
				default: /* unknown sequence, exit */
					state->state = ST_FREE;
					if (isalnum(c))
						return -c;
					return c;
			}
			break;
		case ST_SEQ:
			switch(c) {
				case '0': case '1': case '2': case '3': case '4': case '5':
				case '6': case '8': case '7': case '9':
				case ';':
					/* append */
					state->seq[state->seqi] = c;
					state->seqi++;
					if (state->seqi >= sizeof(state->seq)) {
						state->state = ST_FREE;
						return VT100_SEQ;
					}
					return VT100_SEQ;
				default:
					state->seq[state->seqi] = '\0';
					state->state = ST_FREE;
					return sequence(state, c);
			}
	}
	abort();
	return -1; /* suppress warning */
}

vt100_state_t *vt100_init(vt100_state_t *state, int (*write)(void *handle, const char *s, int len), void *user_data)
{
	if (state == NULL)
		state = malloc(sizeof(vt100_state_t));
	state->write = write;
	state->user_data = user_data;
	state->state = ST_FREE;
	state->maxrow = 0;
	state->maxcol = 0;
	state->ccol = 0;
	state->crow = 0;
	return state;
}
