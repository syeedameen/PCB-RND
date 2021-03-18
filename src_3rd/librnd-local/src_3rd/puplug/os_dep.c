/*
    puplug - portable micro plugin framework
    Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas

    libgpmi - General Package/Module Interface
    Copyright (C) 2005-2007 Patric 'TrueLight' Stout & glx & Tibor 'Igor2' Palinkas

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
#include "os_dep.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "libs.h"
#include "error.h"

void pup_build_lib_path(const char *dir, const char *library_name, char *lib_file_name, char *dep_file_name, int path_size)
{
	/* Os dependent part: the file name of a dynamic lib */
#ifdef PUP_HAVE_DLCOMPAT
		/* With dlcompat, just add dir, file name and extension */
		pup_snprintf(lib_file_name, path_size, "%s/%s.dlc", dir, library_name);
#elif DARWIN
		/* On darwin, it is like unix, only so -> dylib */
		pup_snprintf(lib_file_name, path_size, "%s/%s.dylib", dir, library_name);
#elif PUP_HAVE_LDL
		/* On unix, just add dir, file name and extension */
		pup_snprintf(lib_file_name, path_size, "%s/%s.so", dir, library_name);
#else
		/* Nothing is available */
		lib_file_name[0] = '\0';
#endif

	pup_snprintf(dep_file_name, path_size, "%s/%s.pup", dir, library_name);
}

int pup_load_lib(pup_context_t *pup, pup_plugin_t *library, const char *file_name)
{
#ifdef PUP_HAVE_DYNAMIC_PLUGINS
	char *error_str;
	library->handle = dlopen(file_name, PUP_RTLD_GLOBAL | PUP_RTLD_LAZY);
	if (library->handle == NULL) {
		error_str = malloc(PUP_PATH_MAX + 256);
		pup_snprintf(error_str, PUP_PATH_MAX + 256, "%s on file %s", dlerror(), file_name);
		pup_err_stack_push(pup, pup_err_load_library, pup_err_src_load_lib, error_str);
		free(error_str);
		return 1;
	}
	return 0;
#else
	pup_err_stack_push(pup, pup_err_load_library, pup_err_src_load_lib, "puplug compiled without dynamic loading");
	return 1;
#endif
}

void pup_unload_lib(pup_plugin_t *library)
{
#ifdef PUP_HAVE_DYNAMIC_PLUGINS
	dlclose(library->handle);
#endif
}

void *pup_dlsym(pup_plugin_t *library, const char *name)
{
#ifdef PUP_HAVE_DYNAMIC_PLUGINS
	return dlsym(library->handle, name);
#else
	return NULL;
#endif
}

void *pup_dlopen_global(void)
{
#ifdef PUP_HAVE_DYNAMIC_PLUGINS
	return dlopen(NULL, PUP_RTLD_GLOBAL);
#else
	return NULL;
#endif
}

/* This is an unsafe replacement of snprintf for systems which lack a real one */
#ifndef PUP_HAVE_SNPRINTF
#include <stdarg.h>

int pup_snprintf(char *buf, int len, const char *format, ...)
{
	int ret;
	va_list ap;
	
	va_start(ap, format);
	ret = vsprintf(buf, format, ap);
	va_end(ap);
	return ret;
}
#endif

char *pup_strdup(const char *s)
{
	char *res;
	size_t len = strlen(s);
	res = malloc(len+1);
	if (res == NULL)
		return NULL;
	memcpy(res, s, len+1);
	return res;
}
