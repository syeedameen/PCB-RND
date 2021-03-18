/*
    scconfig - simple c menus
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

#include "hookreadline.h"

typedef unsigned char scm_coord_t;

typedef struct {
	hookreadline_t hrl;
} scm_ctx_t;

int scm_init(scm_ctx_t *ctx);

typedef struct {
	int cfg_x1, cfg_y1, cfg_w, cfg_h; /* desired values */
	int top_only;
	const char *title;
	int active;

	scm_coord_t x1, y1, w, h; /* actual values */
} scm_win_t;

/**** menu ****/
typedef enum {
	SCM_TERMINATOR,
	SCM_KEY_ONLY,
	SCM_KEY_VALUE,
	SCM_KEY_VALUE_EDIT,

	SCM_COMBO,
	SCM_SUBMENU,
	SCM_SUBMENU_CB,

	SCM_EMPTY,
	SCM_SEPARATOR
} scm_entry_type_t;

typedef enum {
	SCM_CHANGED = 1,
	SCM_CHECKED = 2,    /* whether a checkbox is checked */
	SCM_CHECKBOX = 4,   /* whether the entry has a checkbox */
	SCM_RADIO    = 8,   /* whether the entry has a radio button */
	SCM_AUTO_RUN = 16   /* do not return but try to automatically run the menu item clicked */
} scm_menu_entry_flags_t;

typedef struct {
	scm_entry_type_t type;
	char *prefix;
	char *key;
	char *value;
	void *user_data;
	void *auto_data;
	int flags; /* bitfield of scm_menu_entry_flags_t */
} scm_menu_entry_t;

typedef struct scm_menu_s scm_menu_t;
struct scm_menu_s {
	scm_menu_entry_t *entries; /* must be terminated by an SCM_TERMINATOR entry */
	int cursor;    /* cursor in the array */
	int scroll;    /* scroll offset */

	int num_entries; /* calculated by the lib */
	int kwidth;      /* calculated max key width */
	int prev_cursor; /* save of the previous cursor pos for speeding up redraws */
	int is_select;   /* calculated: has radio or checkbox */
	int has_radio;   /* calculated: has radio button */
	int radio_cursor;
};

/* should return the same as scm_menu, or -2 to reiterate */
typedef int (*scm_menu_enter_t)(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win, int selection);

/* Draw and run a menu. If menu_enter is not NULL, call it when enter
   is pressed, else return.
   Return value:
     - on esc, -1 (window is closed if menu_enter==NULL)
     - for plain menus, menu_enter==NULL: cursor position
     - for radio buttons: radio cursor
*/
int scm_menu(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win, scm_menu_enter_t menu_enter);

int scm_menu_autowin(scm_ctx_t *ctx, const char *title, scm_menu_entry_t *entries);


int scm_menu_run(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *win, scm_menu_enter_t menu_enter);


/**** readline ****/

char *scm_readline(scm_ctx_t *ctx, const char *title, const char *prompt, const char *orig);

/**** text and popup ****/

typedef enum {
	SCM_YES    = 1,
	SCM_NO     = 2,
	SCM_OK     = 4,
	SCM_CANCEL = 8,
	SCM_QUIT   = 16,

	SCM_BUTTON_BITS = 5    /* number of button bits */
} scm_button_bits_t;

typedef struct {
	char **row;
	int num_rows;  /* must be filled in before the call */
	int scroll;    /* scroll offset */
	scm_button_bits_t buttons;

	/* internal */
	int num_buttons, bcursor, prev_scroll;
	scm_coord_t button_start;
} scm_text_t;

scm_button_bits_t scm_text(scm_ctx_t *ctx, scm_text_t *text, scm_win_t *w);

scm_button_bits_t scm_popup(scm_ctx_t *ctx, const char *title, const char *text, scm_button_bits_t buttons);


