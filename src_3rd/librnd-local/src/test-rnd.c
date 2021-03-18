/*
 *                            COPYRIGHT
 *
 *  librnd - ringdove EDA lib
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
 *    Project page: http://repo.hu/projects/foobar
 *    lead developer: http://repo.hu/projects/foobar/contact.html
 *    mailing list: foobar (at) list.repo.hu (send "subscribe")
 */

/* the app is called foobar - replace foobar and FOOBAR with your app name */

#include <stdlib.h>
#include <puplug/libs.h>

/* librnd headers */
#include <librnd/core/unit.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hid.h>
#include <librnd/core/conf.h>
#include <librnd/core/buildin.hidlib.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/plugins.h>

/* local (app) headers */
/*#include "data.h"*/

#define pup_buildins foobar_buildins
/*#include "foobar-buildin.c" ; temporary replacement for compiling this test code: */ const pup_buildin_t foobar_buildins[] = { NULL };
#undef pup_buildins
extern const pup_buildin_t pup_buildins[];


const char *pcb_conf_internal = ""; /* should be generated from the conf */
const char *rnd_hidlib_default_embedded_menu = "";
const char *rnd_conf_internal = "";
const char *rnd_menu_file_paths[] = { "./", "~/.foobar/", NULL };
const char *rnd_menu_name_fmt = "foobar-menu.lht";

#define FOOBAR_VERSION "1.0.0"
#define FOOBARSHAREDIR "/usr/share/foobar"
#define CONF_USER_DIR "~/.foobar"
const char *rnd_conf_userdir_path = CONF_USER_DIR;
const char *rnd_pcphl_conf_user_path = CONF_USER_DIR "/foobar-conf.lht";
const char *rnd_conf_sysdir_path = FOOBARSHAREDIR;
const char *rnd_conf_sys_path = FOOBARSHAREDIR "/foobar-conf.lht";
const char *rnd_app_package = "foobar";
const char *rnd_app_version = FOOBAR_VERSION;
const char *rnd_app_url = "http://repo.hu/projects/foobar";

/*** the data model ***/
#include <librnd/core/hidlib.h>
struct {
	rnd_hidlib_t hidlib;
	int whatever_data;
} foobar;

/*** crosshair ***/
#include <librnd/core/hid_inlines.h>
#include <librnd/core/hidlib_conf.h>
rnd_hid_gc_t foobar_crosshair_gc;
void foobar_crosshair_gui_init(void)
{
	foobar_crosshair_gc = rnd_hid_make_gc();
	rnd_hid_set_draw_xor(foobar_crosshair_gc, 1);
	foobar.hidlib.grid = rnd_conf.editor.grid;
}

void foobar_crosshair_gui_uninit(void)
{
	rnd_hid_destroy_gc(foobar_crosshair_gc);
}

void rnd_draw_attached(rnd_hidlib_t *hidlib, rnd_bool inhibit_drawing_mode)
{
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, 1, NULL);
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE_XOR, 1, NULL);

/*	rnd_render->set_color(foobar_crosshair_gc, &conf_core.appearance.color.attached);*/
/*	foobar_tool_draw_attached(hidlib);*/

	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, 1, NULL);
}

void rnd_draw_marks(rnd_hidlib_t *hidlib, rnd_bool inhibit_drawing_mode)
{

}

void rnd_hidlib_crosshair_move_to(rnd_hidlib_t *hl, rnd_coord_t abs_x, rnd_coord_t abs_y, int mouse_mot)
{
	/* do the grid fit/snap then: update the GUI */
	rnd_hid_notify_crosshair_change(hl, rnd_false);
	rnd_gui->set_crosshair(rnd_gui, abs_x, abs_y, 0);
/*	rnd_tool_adjust_attached(hl);*/
	rnd_hid_notify_crosshair_change(hl, rnd_true);
}

void rnd_hidlib_adjust_attached_objects(rnd_hidlib_t *hl)
{
}

void *rnd_hidlib_crosshair_suspend(rnd_hidlib_t *hl)
{
	return NULL;
}

void rnd_hidlib_crosshair_restore(rnd_hidlib_t *hl, void *susp_data)
{
}


/*** draw ***/
void rnd_expose_main(rnd_hid_t *hid, const rnd_hid_expose_ctx_t *region, rnd_xform_t *xform_caller)
{
}

void rnd_expose_preview(rnd_hid_t *hid, const rnd_hid_expose_ctx_t *e)
{
}


/*** gui support ***/
static void gui_support_plugins(int load)
{
	static int loaded = 0;
	static pup_plugin_t *puphand;

	if (load && !loaded) {
		static const char *plugin_name = "foobar-dialogs";
		int state = 0;
		loaded = 1;
		rnd_message(RND_MSG_DEBUG, "Loading GUI support plugin: '%s'\n", plugin_name);
		puphand = pup_load(&rnd_pup, (const char **)rnd_pup_paths, plugin_name, 0, &state);
		if (puphand == NULL)
			rnd_message(RND_MSG_ERROR, "Error: failed to load GUI support plugin '%s'\n-> expect missing widgets and dialog boxes\n", plugin_name);
	}
	if (!load && loaded && (puphand != NULL)) {
		pup_unload(&rnd_pup, puphand, NULL);
		loaded = 0;
		puphand = NULL;
	}
}

/* action table number of columns for a single action */
static const char *foobar_action_args[] = {
/*short, -long, action, help, hint-on-error */
	NULL, "-show-actions",    "PrintActions()",     "Print all available actions (human readable) and exit",   NULL,
	NULL, "-dump-actions",    "DumpActions()",      "Print all available actions (script readable) and exit",  NULL,
	NULL, "-dump-plugins",    "DumpPlugins()",      "Print all available plugins (script readable) and exit",  NULL,
	NULL, "-dump-plugindirs", "DumpPluginDirs()",   "Print directories plugins might be loaded from and exit", NULL,
	NULL, "-dump-oflags",     "DumpObjFlags()",     "Print object flags and exit",                             NULL,
	NULL, "-show-paths",      "PrintPaths()",       "Print all configured paths and exit",                     NULL,
	"V",  "-version",         "PrintVersion()",     "Print version info and exit",                             NULL,
	"V",  "-dump-version",    "DumpVersion()",      "Print version info in script readable format and exit",   NULL,
	NULL, "-copyright",       "PrintCopyright()",   "Print copyright and exit",                                NULL,
	NULL, NULL, NULL, NULL, NULL /* terminator */
};

static void foobar_main_uninit(void)
{
	gui_support_plugins(0);
}

static void foobar_main_init(void)
{
}

void conf_core_init()
{
	/* TODO: in foobar this should be coming from generated and included from conf_core.[ch] */
}

int main(int argc, char *argv[])
{
	int n;
	rnd_main_args_t ga;

	rnd_fix_locale_and_env();

	rnd_main_args_init(&ga, argc, foobar_action_args);

	rnd_hidlib_init1(conf_core_init);
/*	foobar_event_init_app(); - creates all the events */

	for(n = 1; n < argc; n++)
		n += rnd_main_args_add(&ga, argv[n], argv[n+1]);
	rnd_hidlib_init2(pup_buildins, foobar_buildins);

	foobar_main_init();

	/* flipping the "board" means getting a coord system where y increases upward */
	rnd_conf_set(RND_CFR_CLI, "editor/view/flip_y", 0, "1", RND_POL_OVERWRITE);

	if (rnd_main_args_setup1(&ga) != 0) {
		foobar_main_uninit();
		rnd_main_args_uninit(&ga);
		exit(1);
	}

/* Initialize actions only when the gui is already known so only the right
   one is registered (there can be only one GUI). */
/*#include "generated_lists.h"*/

	if (rnd_main_args_setup2(&ga, &n) != 0) {
		foobar_main_uninit();
		rnd_main_args_uninit(&ga);
		exit(n);
	}

	/* foobar: load the design specified on command line */

	if (rnd_main_exported(&ga, &foobar.hidlib, 0)) {
		foobar_main_uninit();
		rnd_main_args_uninit(&ga);
		exit(0);
	}

	foobar_crosshair_gui_init();

	/* main loop */
	if (RND_HAVE_GUI_ATTR_DLG)
		gui_support_plugins(1);

	rnd_mainloop_interactive(&ga, &foobar.hidlib);

	foobar_crosshair_gui_uninit();
	foobar_main_uninit();
	rnd_main_args_uninit(&ga);
	return 0;
}
