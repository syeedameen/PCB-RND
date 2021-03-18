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

#ifndef PUPLUG_OS_DEP_H
#define PUPLUG_OS_DEP_H

/* enable sprintf() if the system has it */
#ifdef PUP_HAVE_SNPRINTF
#	ifndef _DEFAULT_SOURCE
#		define _DEFAULT_SOURCE
#	endif
#	ifndef _BSD_SOURCE
#		define _BSD_SOURCE
#	endif
#endif

/* Define a library handle */
#define pup_handle_t void *

#include <puplug/config.h>
#include <puplug/puplug.h>

#ifdef PUP_HAVE_DLCOMPAT
#	include <dl-compat.h>
#	define PUP_HAVE_DYNAMIC_PLUGINS
#else
#	ifdef PUP_HAVE_LDL
#		include <dlfcn.h>
#		define PUP_HAVE_DYNAMIC_PLUGINS
#	else
#		undef PUP_HAVE_DYNAMIC_PLUGINS
#	endif
#endif


#include <limits.h>
#ifdef PATH_MAX
#	define PUP_PATH_MAX PATH_MAX
#else
#	ifdef MAX_PATH
#		define PUP_PATH_MAX MAX_PATH
#	else
#		define PUP_PATH_MAX 4096
#	endif
#endif

#ifndef PUP_HAVE_SNPRINTF
#include <stdarg.h>
int pup_snprintf(char *buf, int len, const char *format, ...);
#else
#define pup_snprintf  snprintf
#endif

#ifdef RTLD_GLOBAL
#	define PUP_RTLD_GLOBAL RTLD_GLOBAL
#else
#	define PUP_RTLD_GLOBAL 0
#endif

#ifdef RTLD_LAZY
#	define PUP_RTLD_LAZY RTLD_LAZY
#else
#	define PUP_RTLD_LAZY 0
#endif

#include "libs.h"

/* Build full pathname of a library, appending the system-dependent suffix */
void  pup_build_lib_path(const char *dir, const char *library_name, char *lib_file_name, char *pup_file_name, int path_size);

/* portable loading and unloading of a dynamic linked library */
int   pup_load_lib(pup_context_t *pup, pup_plugin_t *library, const char *file_name);
void  pup_unload_lib(pup_plugin_t *library);

/* portable sumbol resolver */
void *pup_dlsym(pup_plugin_t *library, const char *name);

/* portable function to get a handle to the global address space */
void *pup_dlopen_global(void);

/* Portable strdup() */
char *pup_strdup(const char *s);

#endif /* PUPLUG_OS_DEP_H */
