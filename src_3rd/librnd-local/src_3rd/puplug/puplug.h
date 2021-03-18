/*
    puplug - portable micro plugin framework
    Copyright (C) 2017 Tibor 'Igor2' Palinkas

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

#ifndef PUPLUG_H
#define PUPLUG_H


typedef struct pup_err_stack_s pup_err_stack_t;

typedef struct pup_plugin_s pup_plugin_t;
typedef struct pup_buildin_s pup_buildin_t;
typedef struct pup_context_s  pup_context_t;

#include <puplug/libs.h>

struct pup_err_stack_s {
	int error;
	int from;
	char *details;
	pup_err_stack_t *next;
};


struct pup_context_s {
	int error_stack_enable;          /* Controlling the error stack: 0 means disabled, 1 means enabled */
	pup_err_stack_t *err_stack;

	pup_plugin_t *plugins;           /* Linked list of all plugins loaded */

	const pup_buildin_t **bu;        /* array of all builtins available */
	int bu_used, bu_alloced;
};

void pup_init(pup_context_t *pup);
void pup_uninit(pup_context_t *pup);

#endif
