/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2016..2019 Tibor 'Igor2' Palinkas
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
 *
 */

#ifndef PCB_HID_COMMON_MENU_HELPER_H
#define PCB_HID_COMMON_MENU_HELPER_H

/* Looks up an integer (usually boolean) value by conf path or by running
   an action (if name has a parenthesis). When an action is run, it has 0
   or 1 argument only and the return value of the action is returned.
   On error, returns -1. */
int pcb_hid_get_flag(rnd_hidlib_t *hidlib, const char *name);

/* Return non-zero if submenu has further submenus; generate rnd_message(RND_MSG_ERROR, ) if
   there is a submenu field with the wrong lihata type */
int pcb_hid_cfg_has_submenus(const lht_node_t *submenu);

/* Return a lihata node using a relative lihata path from parent - this is
   just a wrapper around lht_tree_path_ */
lht_node_t *pcb_hid_cfg_menu_field_path(const lht_node_t *parent, const char *path);

/* Return a text field of a submenu; return NULL and generate a rnd_message(RND_MSG_ERROR, ) if
   the given field is not text */
const char *pcb_hid_cfg_menu_field_str(const lht_node_t *submenu, pcb_hid_cfg_menufield_t field);

/* Remove a path recursively; call gui_remove() on leaf paths until the subtree
   is consumed (should return 0 on success) */
int pcb_hid_cfg_remove_menu_node(rnd_hid_cfg_t *hr, lht_node_t *root, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx);

#endif
