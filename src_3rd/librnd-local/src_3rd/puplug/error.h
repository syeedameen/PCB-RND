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

/**
 * @file error.h
 * \brief Here are all errors stored, which puplug can return.
 *
 * Remember that no functions have the type 'pup_errors' as return value, but they
 *  can still return an error-code like this. Per function is defined if this is true
 *  yes or no.
 *
 * The optional error stack can be enabled or disabled globally. Each thread
 * has a separate error stack.
 */

#ifndef PUPLUG_ERROR_H
#define PUPLUG_ERROR_H

#include <puplug/puplug.h>

typedef enum pup_error_sources {
	pup_err_src_unknown   = 0,
	pup_err_src_load_dep  = 1,
	pup_err_src_find_lib  = 2,
	pup_err_src_load_pup  = 3,
	pup_err_src_load_lib  = 4
} pup_error_source_t;

/**
 * Errors which you can get back from puplug functions.
 */
typedef enum pup_errors {
	pup_err_success            =  0,         /* Everything is ok */
	pup_err_load_library       = -34001,     /* Failed to load the library (low level) */
	pup_err_load_plugin        = -34002,     /* Failed to load the plugin (high level) */
	pup_err_version            = -34003,     /* Plugin version mismatch */
	pup_err_init_failed        = -34004,     /* Plugin initialization failed */
	pup_err_already_loading    = -34005,     /* Already loading the library */
	pup_err_cyclic_dep         = -34006,     /* Found cyclic plugin dependency (load has been canceled) */
	pup_err_file_not_found     = -34007,     /* File not found */
	pup_err_conflict           = -34008,     /* Plugin-plugin conflict */
	pup_err_parse_pup          = -34009      /* Error parsing the pup file */
} pup_error_t;


/* Allocate and push an entry on the error stack, if it is enabled */
pup_error_t pup_err_stack_push(pup_context_t *pup, pup_error_t error, pup_error_source_t from, const char *details);

/* Remove and return an entry from the stack */
pup_err_stack_t *pup_err_stack_pop(pup_context_t *pup);

/* destroy and free and entry or the whole stack (if entry == NULL) */
void pup_err_stack_destroy(pup_context_t *pup, pup_err_stack_t *entry);

/* Enable/disable the error stack. They both return the previous state (0 = disabled, 1 = enabled) */
int pup_err_stack_enable(pup_context_t *pup);
int pup_err_stack_disable(pup_context_t *pup);

/*** Helper routines: convert codes to strings for error messaged ***/

/* convert an error stack entry to string */
char *pup_str_err_stack_entry(pup_err_stack_t *entry);

/* convert an error code to string */
const char *pup_strerror(pup_error_t errnum);

/* convert an error source to string */
const char *pup_strerror_src(pup_error_source_t srcnum);

/* call back a function with each entry of the error stack, converted to string */
void pup_err_stack_process_str(pup_context_t *pup,void (*callback)(pup_err_stack_t *entry, char *string));

#endif /* PUPLUG_ERROR_H */
