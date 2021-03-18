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

/* track pointers */

#include <stdlib.h>
#include "fungw.h"

void fgw_ptr_reg(fgw_ctx_t *ctx, fgw_arg_t *res, const char *ptr_domain, fgw_type_t ptr_type, void *ptr)
{
	res->type = ptr_type | FGW_PTR;
	res->val.ptr_void = ptr;
	if (ptr != NULL)
		htpp_set(&ctx->ptr_tbl, ptr, (void *)ptr_domain);
}

void fgw_ptr_unreg(fgw_ctx_t *ctx, fgw_arg_t *res, const char *ptr_domain)
{
	htpp_pop(&ctx->ptr_tbl, res->val.ptr_void);
	res->type = FGW_VOID;
	res->val.ptr_void = NULL;
}

int fgw_ptr_in_domain(fgw_ctx_t *ctx, fgw_arg_t *ptr, const char *ptr_domain)
{
	if (!(ptr->type & FGW_PTR))
		return 0;
	return htpp_get(&ctx->ptr_tbl, ptr->val.ptr_void) == ptr_domain;
}
