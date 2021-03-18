/*
    fungw - language-agnostic function gateway
    Copyright (C) 2017  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Project page: http://repo.hu/projects/fungw
    Version control: svn://repo.hu/fungw/trunk
*/

/* Dump the content of a context */

#include <stdio.h>
#include "fungw.h"

void fgw_dump_ctx(fgw_ctx_t *ctx, FILE *f, const char *prefix)
{
	htsp_entry_t *e;

	if (prefix == NULL)
		prefix = "";
	fprintf(f, "%sfungw ctx:\n", prefix);

	fprintf(f, "%s objs\n", prefix);
	for (e = htsp_first(&ctx->obj_tbl); e; e = htsp_next(&ctx->obj_tbl, e)) {
		fgw_obj_t *obj = e->value;
		fprintf(f, "%s  {%s}\n", prefix, obj->name);
	}

	fprintf(f, "%s global functions\n", prefix);
	for (e = htsp_first(&ctx->func_tbl); e; e = htsp_next(&ctx->func_tbl, e)) {
		fgw_func_t *fnc = e->value;
		fprintf(f, "%s  %s in {%s}\n", prefix, e->key, fnc->obj->name);
	}
}
