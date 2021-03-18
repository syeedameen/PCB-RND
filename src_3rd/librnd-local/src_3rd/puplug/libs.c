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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "libs.h"
#include "os_dep.h"
#include "os_dep_fs.h"
#include "error.h"

/**
 *  Search for library file in dir_list.
 *
 * @param library_name The library to find
 * @param file_name Filename of the library (path relative from current)
 * @param dep_file_name Filename of the .pup file (path relative from current)
 */
static const char *pup_find_lib(pup_context_t *pup, const char **dir_list, const char *library_name, char *file_name, char *pup_file_name, int path_max)
{
	const char **dir;
	int fd;

	if (dir_list != NULL)
	for(dir = dir_list; *dir != NULL; dir++) {
		pup_build_lib_path(*dir, library_name, file_name, pup_file_name, path_max);

		/* Check if lib file exists */
		if (*file_name != '\0') {
			fd = open(file_name, O_RDONLY);
			if (fd >= 0) {
				close(fd);
				return *dir;
			}
		}
	}

	/* If we are at the end, exit */
	pup_err_stack_push(pup, pup_err_file_not_found, pup_err_src_find_lib, file_name);
	return NULL;
}


static void free_plugin(pup_plugin_t *library)
{
	free(library->name);
	free(library->path);
	free(library);
}

/* Creates a library and adds it to the global library list. */
static pup_plugin_t *alloc_plugin(pup_context_t *pup, const char *library_name)
{
	pup_plugin_t *library;

	/* Alloc and add new library structure */
	library = calloc(sizeof(pup_plugin_t), 1);

	library->name = pup_strdup(library_name);

	/* Insert new lib at the beginning of the linked list */
	if (pup->plugins != NULL)
		pup->plugins->prev = library;
	library->next = pup->plugins;
	pup->plugins  = library;

	return library;
}

static void pup_unlink_and_free(pup_context_t *pup, pup_plugin_t *library)
{
	pup_plugin_t *tmp;

	if (library != pup->plugins) {
		/* Remove the library pointer from the linked list of libraries */
		for (tmp = pup->plugins; tmp != NULL; tmp = tmp->next)
		{
			if (tmp == library) {
				/* Remove the library from the list */
				if (tmp->prev != NULL)
					tmp->prev->next = tmp->next;
				else
					pup->plugins  = tmp->next;

				if (tmp->next != NULL)
					tmp->next->prev = tmp->prev;

				break;
			}
		}
	}
	else {
		/* first record */
		if (pup->plugins->next != NULL)
			pup->plugins->next->prev = NULL;
		pup->plugins = pup->plugins->next;
	}

	free_plugin(library);
}


static char *lib_entry_name(pup_plugin_t *lib, const char *func, char *buff)
{
	char *end;
	strcpy(buff, "pplg_");
	end = buff+5;
	strcpy(end, func);
	end += strlen(func);
	*end = '_';
	end++;
	strcpy(end, lib->name);
	return buff;
}

/**
 * Initialize a plugin.
 *
 * @param required_version The version the package should have. 0 to disable check.
 */
static int on_load(pup_context_t *pup, pup_plugin_t *library, const pup_buildin_t *bu, int required_version)
{
	if (bu == NULL) {
		char buff[PUP_PATH_MAX];
		library->entry_init      = pup_dlsym(library, lib_entry_name(library, "init", buff));
		library->entry_uninit    = pup_dlsym(library, lib_entry_name(library, "uninit", buff));
		library->entry_check_ver = pup_dlsym(library, lib_entry_name(library, "check_ver", buff));
	}
	else {
		library->entry_init = bu->entry_init;
		library->entry_uninit = bu->entry_uninit;
		library->entry_check_ver = bu->entry_check_ver;
		library->flags |= PUP_FLG_STATIC;
	}

	/* Check version, if we need to */
	if (library->entry_check_ver != NULL && required_version != 0 && library->entry_check_ver(required_version) != 0) {
		/* Version check failed, we unload and quit */
		pup_err_stack_push(pup, pup_err_version, pup_err_src_load_pup, library->name);
		return pup_err_version;
	}

	/* For first load, we run entry_init, if it exists and the package is not static */
	if ((library->entry_init != NULL) && (library->entry_init() != 0)) {
		/* Init failed, we unload and quit */
		pup_err_stack_push(pup, pup_err_init_failed, pup_err_src_load_pup, library->name);
		return pup_err_init_failed;
	}
	return 0;
}

pup_plugin_t *pup_load_(pup_context_t *pup, const char **dir_list, const char *dir, const char *library_name, int req_version, int *state)
{
	char file_name[PUP_PATH_MAX];
	char pup_file_name[PUP_PATH_MAX];
	pup_plugin_t *library;
	int depstat, ret, pup_script_is_file;
	const char *pdir, *pup_script;
	const pup_buildin_t *bu = NULL;

	*pup_file_name = '\0';

	/* Check if we already have this library loaded */
	library = pup_lookup(pup, library_name);

	/* Return if found */
	if (library != NULL) {

		if (library->flags & PUP_FLG_LOADING) {
			if (state != NULL)
				*state = pup_err_already_loading;
			pup_err_stack_push(pup, pup_err_already_loading, pup_err_src_load_pup, library_name);
			return NULL;
		}

		/* Increase the reference counter */
		library->references++;
		if (state != NULL)
			*state = 0;

		return library;
	}


	if (pup_conflict_loaded(pup, library_name)) {
		if (state != NULL)
			*state = pup_err_conflict;
		return NULL;
	}

	/* find a static buildin, if not loading a file from a dir */
	if (dir == NULL)
		bu = pup_buildin_find(pup, library_name);

	if (bu == NULL) {

		/* Find dynamic plugin using the search path; if found, file_name and pup_file_name are set */
		if (dir != NULL) {
			const char *local_dir_list[2];
			local_dir_list[0] = dir;
			local_dir_list[1] = NULL;
			pdir = pup_find_lib(pup, local_dir_list, library_name, file_name, pup_file_name, sizeof(file_name));
		}
		else
			pdir = pup_find_lib(pup, dir_list, library_name, file_name, pup_file_name, sizeof(file_name));

		if (pdir == NULL) {
			if (state != NULL)
				*state = pup_err_file_not_found;
			return NULL;
		}
		pup_script_is_file = 1;
	}
	else {
		pup_script = bu->pup_str;
		pup_script_is_file = 0;
	}

	/* Mark as new library */
	if (state != NULL)
		*state = 1;

	library = alloc_plugin(pup, library_name);
	library->flags = PUP_FLG_LOADING;

	/* First load dependencies */
	if (pup_script_is_file)
		depstat = pup_load_pup_file(pup, dir_list, library, pup_file_name);
	else
		depstat = pup_load_pup_str(pup, dir_list, library, pup_script);

	/* Return if it fails */
	if (depstat < 0) {
		if (state != NULL)
			*state = depstat;
		pup_err_stack_push(pup, pup_err_load_plugin, pup_err_src_load_pup, library->name);
		goto free_lib_and_return_null;
	}

	if (bu == NULL) {
		/* Try to load the library */
		ret = pup_load_lib(pup, library, file_name);
		if (ret) {

			if (state != NULL)
				*state = ret;
			pup_err_stack_push(pup, pup_err_load_plugin, pup_err_src_load_pup, file_name);
			goto free_lib_and_return_null;
		}
		library->path = pup_strdup(file_name);
	}

	/* We have now one reference */
	library->references = 1;

	ret = on_load(pup, library, bu, req_version);
	if (ret != 0) {
		if (state != NULL)
			*state = ret;

		goto free_lib_and_return_null;
	}
	
	/* Library is loaded now */
	library->flags &= ~PUP_FLG_LOADING;
	return library;

free_lib_and_return_null: ;
	pup_unlink_and_free(pup, library);
	return NULL;
}

/**
 * Lowlevel function to load a dynamic library.
 *
 * @param state If not NULL, the content can be: 0: lib already exists, 1: lib loaded, other: error
 */
pup_plugin_t *pup_load(pup_context_t *pup, const char **dir_list, const char *library_name, int req_version, int *state)
{
	pup_plugin_t *res;
	res = pup_load_(pup, dir_list, NULL, library_name, req_version, state);
	if (res != NULL)
		res->flags |= PUP_FLG_TOPLEVEL;
	return res;
}



/**
 * Decrease reference number of a library and unload it when
 * it's not needed anymore. Call uninit() if the library is being unloaded.
 */
void pup_unload(pup_context_t *pup, pup_plugin_t *library, void (*uninit)() )
{
	/* Decrease the reference counter */
	library->references--;

	/* If it hits below 0, something went pretty wrong (an unload without a load) */
	assert(library->references >= 0);

	/* There are still links to this lib, don't unload */
	if (library->references > 0)
		return;

	/* Run uninit, both in static and dynamic mode */
	if (uninit != NULL)
		uninit();

	if (library->entry_uninit != NULL)
		library->entry_uninit();

	/* Unload the deps */
	pup_unload_dep(pup, library);

	/* Some things we don't want to do on static libraries */
	if ((library->flags & PUP_FLG_STATIC) == PUP_FLG_STATIC) {
	}
	else {
		/* Unload the library */
		pup_unload_lib(library);
	}

	pup_unlink_and_free(pup, library);
}

/**
 *  Find a library by name in a linked list. Case sensitive.
 */
pup_plugin_t *pup_lookup(pup_context_t *pup, const char *library_name)
{
	pup_plugin_t *library;

	/* Check if library list is empty */
	if (pup->plugins == NULL) {
		return NULL;
	}

	/* Search name for the linked list */
	for (library = pup->plugins; library != NULL; library = library->next) {
		if ((library->name != NULL) && (strcmp(library_name, library->name) == 0)) {
			return library;
		}
	}

	/* Not found */
	return NULL;
}

static int pup_autoload_bu(pup_context_t *pup, const char **search_dir_list)
{
	int n, cnt = 0;
	for(n = 0; n < pup->bu_used; n++)
		if (pup->bu[n]->autoload)
			if (pup_load(pup, search_dir_list, pup->bu[n]->name, 0, NULL) != NULL)
				cnt++;
	return cnt;
}

int pup_autoload_dir(pup_context_t *pup, const char *dirname, const char **search_dir_list)
{
	int cnt = 0;
	const char *cname;
	void *dir;

	if (dirname == NULL)
		return pup_autoload_bu(pup, search_dir_list);

	if (strlen(dirname) >= PUP_PATH_MAX)
		return -1;

	dir = pup_open_dir(dirname);
	if (dir == NULL)
		return -1;

	while((cname = pup_read_dir(dir)) != NULL) {
		char *end, name[PUP_PATH_MAX], path[2*PUP_PATH_MAX+2];
		pup_plugin_t lib, *res;

		strcpy(name, cname); /* safe: cname is returned by readdir, must not be longer than PUP_PATH_MAX */
		end = strrchr(name, '.');
		if ((end == NULL) || ((strcmp(end, ".pup") != 0) && (strcmp(end, ".PUP") != 0)))
			continue;

		/* check if we need to autoload */
		lib.flags = PUP_FLG_FIND_AUTO;
		sprintf(path, "%s/%s", dirname, name);
		pup_load_pup_file(pup, NULL, &lib, path);
		if (!(lib.flags & PUP_FLG_AUTOLOAD))
			continue;

		*end = '\0';
		/* do not auto-load something we already have (as a buildin or from another dir) */
		if (pup_lookup(pup, name) != NULL)
			continue;

		res = pup_load_(pup, search_dir_list, dirname, name, 0, NULL);
		if (res != NULL) {
			cnt++;
			res->flags |= PUP_FLG_TOPLEVEL;
		}
	}

	pup_close_dir(dir);
	return cnt;
}

int pup_autoload_dirs(pup_context_t *pup, const char **dirnames, const char **search_dir_list)
{
	int cnt = 0, c;
	const char **d;
	for(d = dirnames; *d != NULL; d++) {
		c = pup_autoload_dir(pup, *d, search_dir_list);
		if (c > 0)
			cnt++;
	}
	return cnt;
}

