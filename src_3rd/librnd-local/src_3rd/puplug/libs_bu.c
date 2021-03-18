/*
    puplug - portable micro plugin framework
    Copyright (C) 2017 Tibor 'Igor2' Palinkas

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
#include <string.h>
#include "libs.h"
#include "error.h"

void pup_buildin_load(pup_context_t *pup, const pup_buildin_t *arr)
{
	const pup_buildin_t *b;

	for(b = arr; b->name != NULL; b++) {
		if (pup->bu_used >= pup->bu_alloced) {
			pup->bu_alloced += 16;
			pup->bu = realloc(pup->bu, pup->bu_alloced * sizeof(pup_buildin_t *));
		}
		pup->bu[pup->bu_used] = b;
		pup->bu_used++;
	}
}

const pup_buildin_t *pup_buildin_find(pup_context_t *pup, const char *name)
{
	int n;
	for(n = 0; n < pup->bu_used; n++)
		if (strcmp(pup->bu[n]->name, name) == 0)
			return pup->bu[n];
	return NULL;
}
