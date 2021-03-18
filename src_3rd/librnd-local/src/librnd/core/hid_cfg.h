/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Helpers for loading and handling lihata HID config files */

#ifndef RND_HID_CFG_H
#define RND_HID_CFG_H

#include <liblihata/dom.h>
#include <stdarg.h>
#include <librnd/core/global_typedefs.h>
#include <librnd/core/hid.h>

#define RND_M_Mod0(n)  (1u<<(n))
typedef enum {
	RND_M_Shift   = RND_M_Mod0(0),
	RND_M_Ctrl    = RND_M_Mod0(1),
	RND_M_Alt     = RND_M_Mod0(2),
	RND_M_Mod1    = RND_M_Alt,
	/* RND_M_Mod(3) is RND_M_Mod0(4) */
	/* RND_M_Mod(4) is RND_M_Mod0(5) */
	RND_M_Release = RND_M_Mod0(6), /* mouse button release; there might be a random number of modkeys, but hopefully not this many */

	RND_MB_LEFT   = RND_M_Mod0(7),
	RND_MB_MIDDLE = RND_M_Mod0(8),
	RND_MB_RIGHT  = RND_M_Mod0(9),

/* scroll wheel */
	RND_MB_SCROLL_UP     = RND_M_Mod0(10),
	RND_MB_SCROLL_DOWN   = RND_M_Mod0(11),
	RND_MB_SCROLL_LEFT   = RND_M_Mod0(12),
	RND_MB_SCROLL_RIGHT  = RND_M_Mod0(13)
} rnd_hid_cfg_mod_t;
#undef RND_M_Mod0

#define RND_MB_ANY (RND_MB_LEFT | RND_MB_MIDDLE | RND_MB_RIGHT | RND_MB_SCROLL_UP | RND_MB_SCROLL_DOWN | RND_MB_SCROLL_LEFT | RND_MB_SCROLL_RIGHT)
#define RND_M_ANY  (RND_M_Release-1)

struct rnd_hid_cfg_s {
	lht_doc_t *doc;
};

/* Generic, low level lihata loader */
lht_doc_t *rnd_hid_cfg_load_lht(rnd_hidlib_t *hidlib, const char *filename);
lht_doc_t *rnd_hid_cfg_load_str(const char *text);

/* Generic, low level lihata text value fetch */
const char *rnd_hid_cfg_text_value(lht_doc_t *doc, const char *path);

/* Create a new hash node under parent (optional) and create a flat subtree of
   text nodes from name,value varargs (NULL terminated). This is a shorthand
   for creating a menu item in a subtree list. If ins_after is not NULL and
   is under the same parent, the new menu is inserted after ins_after. */
lht_node_t *rnd_hid_cfg_create_hash_node(lht_node_t *parent, lht_node_t *ins_after, const char *name, ...);

/* Create a flat subtree of text nodes from name,value varargs (NULL
   terminated). This is a shorthand for creating a menu item in a
   subtree list. */
void rnd_hid_cfg_extend_hash_node(lht_node_t *node, ...);
void rnd_hid_cfg_extend_hash_nodev(lht_node_t *node, va_list ap);

/* Search a subtree in depth-first-search manner. Call cb on each node as
   descending. If cb returns non-zero, stop the search and return that value.
   Do all this recursively. */
int rnd_hid_cfg_dfs(lht_node_t *parent, int (*cb)(void *ctx, lht_node_t *n), void *ctx);

/* Report an error about a node */
void rnd_hid_cfg_error(const lht_node_t *node, const char *fmt, ...);


/* for backward compatibility: */
#include "hid_menu.h"

#endif
