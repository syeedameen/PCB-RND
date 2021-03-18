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

#include <librnd/config.h>

#include <liblihata/lihata.h>
#include <liblihata/tree.h>

#include <librnd/core/conf.h>
#include <librnd/core/error.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/core/compat_misc.h>

#include "menu_helper.h"

int pcb_hid_get_flag(rnd_hidlib_t *hidlib, const char *name)
{
	const char *cp;

	if (name == NULL)
		return -1;

	cp = strchr(name, '(');
	if (cp == NULL) {
		rnd_conf_native_t *n = rnd_conf_get_field(name);
		if (n == NULL)
			return -1;
		if ((n->type != RND_CFN_BOOLEAN) || (n->used != 1))
			return -1;
		return n->val.boolean[0];
	}
	else {
		const char *end, *s;
		fgw_arg_t res, argv[2];
		if (cp != NULL) {
			const rnd_action_t *a;
			fgw_func_t *f;
			char buff[256];
			int len, multiarg;
			len = cp - name;
			if (len > sizeof(buff)-1) {
				rnd_message(RND_MSG_ERROR, "hid_get_flag: action name too long: %s()\n", name);
				return -1;
			}
			memcpy(buff, name, len);
			buff[len] = '\0';
			a = rnd_find_action(buff, &f);
			if (!a) {
				rnd_message(RND_MSG_ERROR, "hid_get_flag: no action %s\n", name);
				return -1;
			}
			cp++;
			len = strlen(cp);
			end = NULL;
			multiarg = 0;
			for(s = cp; *s != '\0'; s++) {
				if (*s == ')') {
					end = s;
					break;
				}
				if (*s == ',')
					multiarg = 1;
			}
			if (!multiarg) {
				/* faster but limited way for a single arg */
				if ((len > sizeof(buff)-1) || (end == NULL)) {
					rnd_message(RND_MSG_ERROR, "hid_get_flag: action arg too long or unterminated: %s\n", name);
					return -1;
				}
				len = end - cp;
				memcpy(buff, cp, len);
				buff[len] = '\0';
				argv[0].type = FGW_FUNC;
				argv[0].val.argv0.func = f;
				argv[0].val.argv0.user_call_ctx = hidlib;
				argv[1].type = FGW_STR;
				argv[1].val.str = buff;
				res.type = FGW_INVALID;
				if (rnd_actionv_(f, &res, (len > 0) ? 2 : 1, argv) != 0)
					return -1;
				fgw_arg_conv(&rnd_fgw, &res, FGW_INT);
				return res.val.nat_int;
			}
			else {
				/* slower but more generic way */
				return rnd_parse_command(hidlib, name, rnd_true);
			}
		}
		else {
			fprintf(stderr, "ERROR: pcb_hid_get_flag(%s) - not a path or an action\n", name);
		}
	}
	return -1;
}

const char *pcb_hid_cfg_menu_field_str(const lht_node_t *submenu, pcb_hid_cfg_menufield_t field)
{
	const char *fldname;
	lht_node_t *n = pcb_hid_cfg_menu_field(submenu, field, &fldname);

	if (n == NULL)
		return NULL;
	if (n->type != LHT_TEXT) {
		rnd_hid_cfg_error(submenu, "Error: field %s should be a text node\n", fldname);
		return NULL;
	}
	return n->data.text.value;
}

int pcb_hid_cfg_has_submenus(const lht_node_t *submenu)
{
	const char *fldname;
	lht_node_t *n = pcb_hid_cfg_menu_field(submenu, PCB_MF_SUBMENU, &fldname);
	if (n == NULL)
		return 0;
	if (n->type != LHT_LIST) {
		rnd_hid_cfg_error(submenu, "Error: field %s should be a list (of submenus)\n", fldname);
		return 0;
	}
	return 1;
}

lht_node_t *pcb_hid_cfg_menu_field_path(const lht_node_t *parent, const char *path)
{
	return lht_tree_path_(parent->doc, parent, path, 1, 0, NULL);
}

int pcb_hid_cfg_remove_menu_node(rnd_hid_cfg_t *hr, lht_node_t *root, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx)
{
	if ((root == NULL) || (hr == NULL))
		return -1;

	if ((root->type != LHT_TEXT) && (root->type != LHT_HASH)) /* allow text for the sep */
		return -1;

	if (gui_remove(ctx, root) != 0)
		return -1;
	return 0;
}

