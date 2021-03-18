/*
    puplug - portable micro plugin framework
    Copyright (C) 2017 Tibor 'Igor2' Palinkas

    libgpmi - General Package/Module Interface
    Copyright (C) 2005-2007 Patric 'TrueLight' Stout & Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef PUPLUG_LIBS_H
#define PUPLUG_LIBS_H

#include <puplug/puplug.h>
#include <puplug/os_dep.h>

typedef enum pup_plugin_flags {
	PUP_FLG_LOADING   = 1, /* set during the load of a library - used to detect dependency loops */
	PUP_FLG_STATIC    = 2, /* static library - linked in, do not use dlsym */
	PUP_FLG_FIND_AUTO = 4, /* dry run for autoload flags */
	PUP_FLG_AUTOLOAD  = 8, /* library has autoload on */
	PUP_FLG_TOPLEVEL  = 16 /* loaded by the application, not as a dependency */
} pup_plugin_flags_t;

/* Package procedures */
typedef int  pup_check_ver_t(int version_we_need);
typedef int  pup_init_t(void);
typedef void pup_uninit_t(void);

/* The pup_plugin_t struct */
struct pup_plugin_s {
	char *name;                       /* Name of opened library */
	char *path;                       /* Full path the lib was loaded from or NULL for builtin */
	pup_handle_t handle;              /* Handle(s) of current library (UNIX: null of no library) */

	int references;                   /* Reference counter; when drops to 0, lib is unloaded */

	pup_plugin_t *next;               /* Next element in the double linked list of loaded libraries */
	pup_plugin_t *prev;               /* Previous element in the double linked list of loaded libraries */
	pup_plugin_flags_t flags;         /* See the following enum */
	pup_plugin_t *static_package;     /* Static package structure if the library is a static package */

	int deps_len;
	pup_plugin_t **deps;              /* array of packages loaded as dependency */

	int conflict_len;
	char **conflict;                  /* array of conflict package names */

	/* Entry points */
	pup_init_t     *entry_init;       /* Proc to initialize function */
	pup_uninit_t   *entry_uninit;     /* Proc to uninitialize function */
	pup_check_ver_t *entry_check_ver; /* Proc to check version */
};

struct pup_buildin_s {
	char *name;
	pup_init_t     *entry_init;
	pup_uninit_t   *entry_uninit;
	pup_check_ver_t *entry_check_ver;
	int autoload;
	char *pup_str;
};

void pup_buildin_load(pup_context_t *pup, const pup_buildin_t *arr);

/* Low level functions */
pup_plugin_t *pup_load(pup_context_t *pup, const char **dir_list, const char *library_name, int req_version, int *state);
void pup_unload(pup_context_t *pup, pup_plugin_t *library, void (*uninit)(void) );
pup_plugin_t *pup_lookup(pup_context_t *pup, const char *library_name);
void pup_unload_dep(pup_context_t *pup, pup_plugin_t *lib);

/* Load, parse and apply a pup script from file or string */
int pup_load_pup_file(pup_context_t *pup, const char **dir_list, pup_plugin_t *lib, const char *pup_file_name);
int pup_load_pup_str(pup_context_t *pup, const char **dir_list, pup_plugin_t *lib, const char *pup_script);

const pup_buildin_t *pup_buildin_find(pup_context_t *pup, const char *name);

int pup_autoload_dir(pup_context_t *pup, const char *dirname, const char **search_dir_list);
int pup_autoload_dirs(pup_context_t *pup, const char **dirnames, const char **search_dir_list);

/* check if any of the existing plugins would conflict with lib */
int pup_conflict_loaded(pup_context_t *pup, const char *new_name);


/*** Internal ***/
pup_plugin_t *pup_load_(pup_context_t *pup, const char **dir_list, const char *dir, const char *library_name, int req_version, int *state);

#endif /* PUPLUG_LIBS_H */
