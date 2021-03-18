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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "error.h"
#include "os_dep.h"

const char *pup_error_sources_txt[] = {
	"Unknown",
	"load_dep",
	"find_lib",
	"load_pup",
	"load_lib",
	NULL
};

const char *pup_error_txt[] = {
	NULL,
	"Failed to load library",
	"Failed to load plugin",
	"Version mismatch",
	"Initialization failed",
	"Already loading library",
	"Found cyclic dependency",
	"File not found",
	"Plugin-plugin conflict",
	"Error parsing the pup file"
};

#define array_len(array) (sizeof(array) / sizeof(char *))

const char *pup_strerror(pup_error_t errnum)
{
	/* Check if it is a puplug error */
	if ((errnum > -34000) || (errnum < -34000 - array_len(pup_error_txt)))
		return NULL;

	return pup_error_txt[(-errnum) - 34000];
}

const char *pup_strerror_src(pup_error_source_t srcnum)
{
	/* Check if it is a puplug error */
	if ((srcnum < 0) || (srcnum >= array_len(pup_error_sources_txt)))
		return NULL;

	return pup_error_sources_txt[srcnum];
}

pup_error_t pup_err_stack_push(pup_context_t *pup, pup_error_t error, pup_error_source_t from, const char *details)
{
	pup_err_stack_t *entry;

	/* Don't push anything if the stack is not enabled */
	if (!pup->error_stack_enable)
		return error;

	/* Create the entry */
	entry = malloc(sizeof(pup_err_stack_t));
	entry->error = error;
	entry->from  = from;
	if (details != NULL)
		entry->details = pup_strdup(details);
	else
		entry->details = NULL;

	/* Insert at the beginning of the linked list */
	entry->next     = pup->err_stack;
	pup->err_stack = entry;
	return error;
}

pup_err_stack_t *pup_err_stack_pop(pup_context_t *pup)
{
	pup_err_stack_t *entry;

	/* Get the first record */
	entry = pup->err_stack;

	/* Set next record so we popped the first */
	if (entry != NULL)
		pup->err_stack = entry->next;

	return entry;
}

void pup_err_stack_destroy(pup_context_t *pup, pup_err_stack_t *entry)
{
	pup_err_stack_t *e, *next;

	if (entry == NULL) {
		/* Destroy the whole stack */
		for (e = pup->err_stack; e != NULL; e = next) {
			next = e->next;
			pup_err_stack_destroy(pup, e);
		}
		pup->err_stack = NULL;
	}
	else {
		/* Destroy one entry */
		if (entry->details != NULL)
			free(entry->details);

		free(entry);
	}
}

char *pup_str_err_stack_entry(pup_err_stack_t *entry)
{
	char *tmp, *det;
	const char *from, *msg;

	msg = pup_strerror(entry->error);
	if (msg == NULL)
		msg = "(unknown)";

	from = pup_strerror_src(entry->from);
	if (from == NULL)
		from = "(unknown)";

	if (entry->details != NULL)
		det = entry->details;
	else
		det = "(empty)";

	tmp = malloc(strlen(msg) + strlen(from) + strlen(det) + 32);
	sprintf(tmp, "%s(): '%s': '%s'", from, msg, det);
	return tmp;
}

void pup_err_stack_process_str(pup_context_t *pup, void (*callback)(pup_err_stack_t *entry, char *string))
{
	pup_err_stack_t *entry;
	char *str;

	assert(callback != NULL);
	do {
		entry = pup_err_stack_pop(pup);
		if (entry != NULL) {
			str   = pup_str_err_stack_entry(entry);
			callback(entry, str);
			free(str);
			pup_err_stack_destroy(pup, entry);
		}
	} while(entry != NULL);
}

int pup_err_stack_enable(pup_context_t *pup)
{
	int previous;

	previous = pup->error_stack_enable;
	pup->error_stack_enable = 1;
	return previous;
}

int pup_err_stack_disable(pup_context_t *pup)
{
	int previous;

	previous = pup->error_stack_enable;
	pup->error_stack_enable = 0;
	return previous;
}
