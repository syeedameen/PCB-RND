/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include <librnd/config.h>

#include <librnd/core/conf.h>
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/grid.h>
#include <librnd/core/event.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/core/hid.h>
#include <librnd/core/hid_menu.h>

#include "grid_menu.h"

#define ANCH "/anchored/@grid"

static rnd_conf_resolve_t grids_idx = {"editor.grids_idx", RND_CFN_INTEGER, 0, NULL};

void rnd_grid_install_menu(void)
{
	rnd_conf_native_t *nat;
	rnd_conflist_t *lst;
	rnd_conf_listitem_t *li;
	rnd_menu_prop_t props;
	char act[256], chk[256];
	int idx, plen;
	gds_t path = {0};

	nat = rnd_conf_get_field("editor/grids");
	if (nat == NULL)
		return;

	if (nat->type != RND_CFN_LIST) {
		rnd_message(RND_MSG_ERROR, "grid_install_menu(): conf node editor/grids should be a list\n");
		return;
	}

	lst = nat->val.list;

	rnd_conf_resolve(&grids_idx);

	memset(&props, 0,sizeof(props));
	props.action = act;
	props.checked = chk;
	props.update_on = "editor/grids_idx";
	props.cookie = "lib_hid_common grid";

	rnd_hid_menu_merge_inhibit_inc();
	rnd_hid_menu_unload(rnd_gui, props.cookie);

	/* prepare for appending the strings at the end of the path, "under" the anchor */
	gds_append_str(&path, ANCH);
	gds_append(&path, '/');
	plen = path.used;

	/* have to go reverse to keep order because this will insert items */
	idx = rnd_conflist_length(lst)-1;
	for(li = rnd_conflist_last(lst); li != NULL; li = rnd_conflist_prev(li),idx--) {
		sprintf(act, "grid(#%d)", idx);
		sprintf(chk, "conf(iseq, editor/grids_idx, %d)", idx);

		gds_truncate(&path, plen);
		gds_append_str(&path, li->val.string[0]);
		rnd_hid_menu_create(path.array, &props);
	}

	rnd_hid_menu_merge_inhibit_dec();
	gds_uninit(&path);
}

static int grid_lock = 0;

void pcb_grid_update_conf(rnd_conf_native_t *cfg, int arr_idx)
{
	if (grid_lock) return;
	grid_lock++;
	rnd_grid_install_menu();
	grid_lock--;
}

void pcb_grid_update_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (grid_lock) return;
	grid_lock++;
	rnd_grid_install_menu();

	/* to get the right menu checked */
	if ((grids_idx.nat != NULL) && (grids_idx.nat->val.integer[0] >= 0))
		rnd_grid_list_step(hidlib, 0);
	grid_lock--;
}


