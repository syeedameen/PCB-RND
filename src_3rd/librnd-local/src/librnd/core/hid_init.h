/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
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

#ifndef RND_HID_INIT_H
#define RND_HID_INIT_H

#include <puplug/puplug.h>
#include <genvector/vtp0.h>
#include <librnd/core/hid.h>

#define RND_ACTION_ARGS_WIDTH 5

/* NULL terminated list of all static HID structures.  Built by
   hid_register_hid, used by hid_find_*() and rnd_hid_enumerate().  The
   order in this list is the same as the order of hid_register_hid
   calls.  */
extern rnd_hid_t **rnd_hid_list;

/* set to 1 when librnd is running the main loop */
extern int rnd_hid_in_main_loop;

/* Count of entries in the above.  */
extern int rnd_hid_num_hids;

/* Call this as soon as possible from main().  No other HID calls are
   valid until this is called.  */
void rnd_hid_init(void);

/* Call this at exit */
void rnd_hid_uninit(void);

/* When PCB runs in interactive mode, this is called to instantiate
   one GUI HID which happens to be the GUI.  This HID is the one that
   interacts with the mouse and keyboard.  */
rnd_hid_t *rnd_hid_find_gui(rnd_hidlib_t *hidlib, const char *preference);

/* Finds the one printer HID and instantiates it.  */
rnd_hid_t *rnd_hid_find_printer(void);

/* Finds the indicated exporter HID and instantiates it.  */
rnd_hid_t *rnd_hid_find_exporter(const char *);

/* This returns a NULL-terminated array of available HIDs.  The only
   real reason to use this is to locate all the export-style HIDs. */
rnd_hid_t **rnd_hid_enumerate(void);

/* HID internal interfaces.  These may ONLY be called from the HID
   modules, not from the common PCB code.  */

/* A HID may use this if it does not need command line arguments in
   any special format; for example, the Lesstif HID needs to use the
   Xt parser, but the Postscript HID can use this function.  */
int rnd_hid_parse_command_line(int *argc, char ***argv);

/* Called by the init funcs, used to set up rnd_hid_list.  */
extern void rnd_hid_register_hid(rnd_hid_t *hid);
void rnd_hid_remove_hid(rnd_hid_t *hid);

typedef struct rnd_plugin_dir_s rnd_plugin_dir_t;
struct rnd_plugin_dir_s {
	char *path;
	int num_plugins;
	rnd_plugin_dir_t *next;
};

extern rnd_plugin_dir_t *rnd_plugin_dir_first, *rnd_plugin_dir_last;

/* Safe file name for inclusion in export file comments/headers; if the
   user requested in the config, this becomes the basename of filename,
   else it is the full file name */
const char *rnd_hid_export_fn(const char *filename);

/* Config directories the caller needs to load before the conf system is initialized */
extern char *rnd_conf_dot_dir, *rnd_conf_lib_dir;


/*** main(), initialize ***/

typedef struct {
	enum {
		RND_DO_SOMETHING,
		RND_DO_PRINT,
		RND_DO_EXPORT,
		RND_DO_GUI
	} do_what;
	int hid_argc;
	const char *main_action, *main_action_hint;
	vtp0_t plugin_cli_conf;
	char **hid_argv_orig, **hid_argv;
	const char *hid_name;
	const char **action_args;
	int autopick_gui;
} rnd_main_args_t;

/* call this before anything, to switch locale to "C" permanently;
   also sets up the environment on win32. */
void rnd_fix_locale_and_env();

void rnd_hidlib_init1(void (*conf_core_init)(void)); /* before CLI argument parsing; conf_core_init should conf_reg() at least the hidlib related nodes */
void rnd_hidlib_init2(const pup_buildin_t *buildins, const pup_buildin_t *local_buildins); /* after CLI argument parsing */
void rnd_hidlib_init3_auto(void);
void rnd_hidlib_uninit(void);

/* optional: hidlib aspects of main() */
void rnd_main_args_init(rnd_main_args_t *ga, int argc, const char **action_args);
void rnd_main_args_uninit(rnd_main_args_t *ga);

/* Parse the next argument using the next two argv[]s. Returns 0 if arg is
   not consumed, 1 if consimed. */
int rnd_main_args_add(rnd_main_args_t *ga, char *cmd, char *arg);

/* returns non-zero on error (the application should exit); exitval is the
   desired exit value of the executable when setup2 fails. */
int rnd_main_args_setup1(rnd_main_args_t *ga);
int rnd_main_args_setup2(rnd_main_args_t *ga, int *exitval);

/* if -x was specified, do the export and return 1 (the caller should
   exit); else return 0 */
int rnd_main_exported(rnd_main_args_t *ga, rnd_hidlib_t *hidlib, rnd_bool is_empty);

/* launches the GUI or CLI; after it returns, if rnd_gui is not NULL, the user
   has selected another GUI to switch to. */
void rnd_mainloop_interactive(rnd_main_args_t *ga, rnd_hidlib_t *hidlib);

/* parse arguments using the gui; if fails and fallback is enabled, try the next gui */
int rnd_gui_parse_arguments(int autopick_gui, int *hid_argc, char **hid_argv[]);

/* true if main() is called for printing or exporting (no interactive HID
   shall be invoked) */
#define rnd_main_exporting (rnd_gui->printer || rnd_gui->exporter)

#ifdef __WIN32__
extern char *rnd_w32_root;     /* installation prefix; what would be $PREFIX on FHS, e.g. /usr/local */
extern char *rnd_w32_libdir;   /* on FHS this would be $PREFIX/lib*/
extern char *rnd_w32_bindir;   /* on FHS this would be $PREFIX/bin - on win32 this also hosts the dlls */
extern char *rnd_w32_sharedir; /* on FHS this would be $PREFIX/share */
extern char *rnd_w32_cachedir; /* where to store cache files, e.g. gdk pixbuf loader cache; persistent, but not part of the distribution */
#endif

#endif
