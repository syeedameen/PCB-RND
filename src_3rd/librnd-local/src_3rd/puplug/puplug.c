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

#include <stdlib.h>
#include <string.h>
#include "puplug.h"
#include "error.h"

void pup_init(pup_context_t *pup)
{
	memset(pup, 0, sizeof(pup_context_t));
}

void pup_uninit(pup_context_t *pup)
{
	pup_plugin_t *n, *next;

	/* unload all top-level loaded libs */
	for(n = pup->plugins; n != NULL; n = next) {
		next = n->next;
		if (n->flags & PUP_FLG_TOPLEVEL) {
			n->flags &= ~PUP_FLG_TOPLEVEL;
			pup_unload(pup, n, NULL);
		}
	}

	pup_err_stack_destroy(pup, pup->err_stack);
	free(pup->bu);
}
