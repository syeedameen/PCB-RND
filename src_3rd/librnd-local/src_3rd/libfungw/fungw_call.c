/*
    fungw - language-agnostic function gateway
    Copyright (C) 2017, 2018, 2019  Tibor 'Igor2' Palinkas

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

/* C->anything call helpers */

#include <stdlib.h>
#include <stdarg.h>
#include "fungw.h"

static char *call_func(fgw_ctx_t *ctx, int need_retval, int argc, fgw_arg_t *argv)
{
	fgw_arg_t res = {0};
	if (argv[0].val.func->func(&res, argc, argv) != 0)
		return NULL;
	if (!need_retval) {
		if (res.type & FGW_DYN)
			fgw_arg_free(ctx, &res);
		return NULL;
	}
	fgw_arg_conv(ctx, &res, FGW_STR | FGW_DYN);
	return res.val.str;
}


#define ARG_DECL \
	va_list ap; \
	int argc; \
	fgw_arg_t argv_static[16], *argv; \

#ifdef FUNGW_CFG_LONG
#	define ARG_GET_TYPED_LONG(typ, dst) \
			case FGW_LLONG:   dst.nat_llong = va_arg(ap, long long); break; \
			case FGW_ULLONG:  dst.nat_ullong = va_arg(ap, unsigned long long); break; \
			case FGW_LDOUBLE: dst.nat_ldouble = va_arg(ap, long double); break;
#else
#	define ARG_GET_TYPED_LONG(typ, dst)
#endif

#define ARG_GET_TYPED(typ, dst) \
	do { \
		if (typ & FGW_PTR) \
			dst.ptr_void = va_arg(ap, void *); \
		else switch(typ) { \
			case FGW_CHAR:    dst.nat_char = va_arg(ap, int); break; \
			case FGW_UCHAR:   dst.nat_uchar = va_arg(ap, int); break; \
			case FGW_SCHAR:   dst.nat_schar = va_arg(ap, int); break; \
			case FGW_SHORT:   dst.nat_short = va_arg(ap, int); break; \
			case FGW_USHORT:  dst.nat_ushort = va_arg(ap, int); break; \
			case FGW_INT:     dst.nat_int = va_arg(ap, int); break; \
			case FGW_UINT:    dst.nat_uint = va_arg(ap, unsigned int); break; \
			case FGW_LONG:    dst.nat_long = va_arg(ap, long); break; \
			case FGW_ULONG:   dst.nat_ulong = va_arg(ap, unsigned long); break; \
			case FGW_SIZE_T:  dst.nat_size_t = va_arg(ap, size_t); break; \
			case FGW_FLOAT:   dst.nat_float = va_arg(ap, double); break; \
			case FGW_DOUBLE:  dst.nat_double = va_arg(ap, double); break; \
			ARG_GET_TYPED_LONG(typ, dst) \
			case FGW_VOID: \
			case FGW_STRUCT: \
			case FGW_TERM: \
			case FGW_FUNC: \
			case FGW_PTR: \
			case FGW_STR: \
			case FGW_ZTERM: \
			case FGW_DYN: \
			case FGW_CUSTOM: \
			case FGW_AUTO: \
				goto badarg; \
		} \
	} while(0)

#define ARG_PRE_COMMON(dbl_get) \
	va_start(ap, func_name); \
	if (dbl_get) { \
		fgw_arg_t tmp; \
		for(argc = 1; (tmp.type = va_arg(ap, fgw_type_t)) != 0; argc++) { \
			ARG_GET_TYPED(tmp.type, tmp.val); \
		} \
	} \
	else \
		for(argc = 1; va_arg(ap, char *) != NULL; argc++) ; \
	va_end(ap); \
	\
	if ((argc) > (sizeof(argv_static) / sizeof(argv_static[0]))) \
		argv = malloc((argc) * sizeof(fgw_arg_t)); \
	else \
		argv = argv_static; \
	\
	argv[0].type = FGW_FUNC; \

#define ARG_PRE_STR \
	do { \
		int n; \
		\
		ARG_PRE_COMMON(0) \
		\
		va_start(ap, func_name); \
		for(n = 1; n < argc; n++) { \
			argv[n].type = FGW_STR; \
			argv[n].val.str = va_arg(ap, char *); \
		} \
		va_end(ap); \
	} while(0)

#define ARG_PRE_TYPE_DATA \
	do { \
		int n; \
		\
		ARG_PRE_COMMON(1) \
		\
		va_start(ap, func_name); \
		for(n = 1; n < argc; n++) { \
			argv[n].type = va_arg(ap, fgw_type_t); \
			ARG_GET_TYPED(argv[n].type, argv[n].val); \
		} \
		va_end(ap); \
	} while(0)

#define ARG_POST \
	do { \
		goto badarg; \
		badarg:; \
		if (argv != argv_static) \
			free(argv); \
	} while(0)

/* Makes a static copy of all arguments so in_argv[] is preserved
   (argument conversion will not affect its entries) */
static char *call_func_retain(fgw_ctx_t *ctx, int need_retval, int in_argc, const fgw_arg_t *in_argv)
{
	int n;
	char *res;
	fgw_arg_t argv_static[16], *argv;

	if ((in_argc) > (sizeof(argv_static) / sizeof(argv_static[0])))
		argv = malloc((in_argc) * sizeof(fgw_arg_t));
	else
		argv = argv_static;

	for(n = 0; n < in_argc; n++) {
		argv[n] = in_argv[n];
		argv[n].type &= ~FGW_DYN;
	}

	res = call_func(ctx, need_retval, in_argc, argv);

	ARG_POST;
	return res;
}


char *fgw_scall(fgw_ctx_t *ctx, const char *func_name, ...)
{
	char *res = NULL;
	fgw_func_t *f;
	ARG_DECL;
	f = fgw_func_lookup(ctx, func_name);
	if (f == NULL)
		return NULL;

	ARG_PRE_STR;

	argv[0].val.argv0.func = f;
	argv[0].val.argv0.user_call_ctx = NULL;
	res = call_func(ctx, 1, argc, argv);

	fgw_argv_free(ctx, argc, argv);
	ARG_POST;

	return res;
}

char *fgw_uscall(fgw_ctx_t *ctx, void *user_call_ctx, const char *func_name, ...)
{
	char *res = NULL;
	fgw_func_t *f;
	ARG_DECL;
	f = fgw_func_lookup(ctx, func_name);
	if (f == NULL)
		return NULL;

	ARG_PRE_STR;

	argv[0].val.argv0.func = f;
	argv[0].val.argv0.user_call_ctx = user_call_ctx;
	res = call_func(ctx, 1, argc, argv);

	fgw_argv_free(ctx, argc, argv);
	ARG_POST;

	return res;
}

	/* calulate whether this is a multi-call */
static int is_multi(fgw_ctx_t *ctx, const char *func_name)
{
	htsp_entry_t *e;
	int multi = 0;

	for (e = htsp_first(&ctx->obj_tbl); e; e = htsp_next(&ctx->obj_tbl, e)) {
		fgw_obj_t *l = e->value;
		fgw_func_t *f = htsp_get(&l->func_tbl, func_name);
		if (f == NULL)
			continue;
		multi++;
		if (multi > 1)
			return 1;
	}
	return 0;
}

void fgw_scall_all(fgw_ctx_t *ctx, const char *func_name, ...)
{
	htsp_entry_t *e;
	int multi = is_multi(ctx, func_name);
	ARG_DECL;
	ARG_PRE_STR;

	/* look up all matching names in all objects */
	for (e = htsp_first(&ctx->obj_tbl); e; e = htsp_next(&ctx->obj_tbl, e)) {
		fgw_obj_t *l = e->value;
		fgw_func_t *f = htsp_get(&l->func_tbl, func_name);
		if (f == NULL)
			continue;
		argv[0].val.argv0.func = f;
		argv[0].val.argv0.user_call_ctx = NULL;
		if (multi)
			call_func_retain(ctx, 0, argc, argv);
		else
			call_func(ctx, 0, argc, argv);
		/* no need to fgw_argv_free() - all arguments aer non-dynamic strings */
	}

	fgw_argv_free(ctx, argc, argv);
	ARG_POST;
}

void fgw_uscall_all(fgw_ctx_t *ctx, void *user_call_ctx, const char *func_name, ...)
{
	htsp_entry_t *e;
	int multi = is_multi(ctx, func_name);
	ARG_DECL;
	ARG_PRE_STR;

	/* look up all matching names in all objects */
	for (e = htsp_first(&ctx->obj_tbl); e; e = htsp_next(&ctx->obj_tbl, e)) {
		fgw_obj_t *l = e->value;
		fgw_func_t *f = htsp_get(&l->func_tbl, func_name);
		if (f == NULL)
			continue;
		argv[0].val.argv0.func = f;
		argv[0].val.argv0.user_call_ctx = user_call_ctx;
		if (multi)
			call_func_retain(ctx, 0, argc, argv);
		else
			call_func(ctx, 0, argc, argv);
		/* no need to fgw_argv_free() - all arguments aer non-dynamic strings */
	}

	fgw_argv_free(ctx, argc, argv);
	ARG_POST;
}

fgw_error_t fgw_vcall(fgw_ctx_t *ctx, fgw_arg_t *res, const char *func_name, ...)
{
	fgw_error_t err = FGW_ERR_UNKNOWN;
	fgw_func_t *f;
	ARG_DECL;
	f = fgw_func_lookup(ctx, func_name);
	if (f == NULL)
		return FGW_ERR_NOT_FOUND;

	ARG_PRE_TYPE_DATA;

	argv[0].val.argv0.func = f;
	argv[0].val.argv0.user_call_ctx = NULL;
	err = f->func(res, argc, argv);

	fgw_argv_free(ctx, argc, argv);
	ARG_POST;

	return err;
}

fgw_error_t fgw_uvcall(fgw_ctx_t *ctx, void *user_call_ctx, fgw_arg_t *res, const char *func_name, ...)
{
	fgw_error_t err = FGW_ERR_UNKNOWN;
	fgw_func_t *f;
	ARG_DECL;
	f = fgw_func_lookup(ctx, func_name);
	if (f == NULL)
		return FGW_ERR_NOT_FOUND;

	ARG_PRE_TYPE_DATA;

	argv[0].val.argv0.func = f;
	argv[0].val.argv0.user_call_ctx = user_call_ctx;
	err = f->func(res, argc, argv);

	fgw_argv_free(ctx, argc, argv);
	ARG_POST;

	return err;
}



fgw_error_t fgw_vcall_in(fgw_ctx_t *ctx, fgw_arg_t *res, const char *obj_name, const char *func_name, ...)
{
	fgw_error_t err = FGW_ERR_UNKNOWN;
	fgw_obj_t *obj;
	fgw_func_t *f;
	ARG_DECL;
	obj = fgw_obj_lookup(ctx, obj_name);
	if (obj == NULL)
		return FGW_ERR_NOT_FOUND;
	f = fgw_func_lookup_in(obj, func_name);
	if (f == NULL)
		return FGW_ERR_NOT_FOUND;

	ARG_PRE_TYPE_DATA;

	argv[0].val.argv0.func = f;
	argv[0].val.argv0.user_call_ctx = NULL;
	err = f->func(res, argc, argv);

	fgw_argv_free(ctx, argc, argv);
	ARG_POST;

	return err;
}

fgw_error_t fgw_uvcall_in(fgw_ctx_t *ctx, void *user_call_ctx, fgw_arg_t *res, const char *obj_name, const char *func_name, ...)
{
	fgw_error_t err = FGW_ERR_UNKNOWN;
	fgw_obj_t *obj;
	fgw_func_t *f;
	ARG_DECL;
	obj = fgw_obj_lookup(ctx, obj_name);
	if (obj == NULL)
		return FGW_ERR_NOT_FOUND;
	f = fgw_func_lookup_in(obj, func_name);
	if (f == NULL)
		return FGW_ERR_NOT_FOUND;

	ARG_PRE_TYPE_DATA;

	argv[0].val.argv0.func = f;
	argv[0].val.argv0.user_call_ctx = user_call_ctx;
	err = f->func(res, argc, argv);

	fgw_argv_free(ctx, argc, argv);
	ARG_POST;

	return err;
}


void fgw_vcall_all(fgw_ctx_t *ctx, const char *func_name, ...)
{
	htsp_entry_t *e;
	int multi = is_multi(ctx, func_name);
	ARG_DECL;
	ARG_PRE_TYPE_DATA;

	/* look up all matching names in all objects */
	for (e = htsp_first(&ctx->obj_tbl); e; e = htsp_next(&ctx->obj_tbl, e)) {
		fgw_obj_t *l = e->value;
		fgw_func_t *f = htsp_get(&l->func_tbl, func_name);
		if (f == NULL)
			continue;
		argv[0].val.argv0.func = f;
		argv[0].val.argv0.user_call_ctx = NULL;
		if (multi)
			call_func_retain(ctx, 0, argc, argv);
		else
			call_func(ctx, 0, argc, argv);
	}

	fgw_argv_free(ctx, argc, argv);
	ARG_POST;
}

void fgw_uvcall_all(fgw_ctx_t *ctx, void *user_call_ctx, const char *func_name, ...)
{
	htsp_entry_t *e;
	int multi = is_multi(ctx, func_name);
	ARG_DECL;
	ARG_PRE_TYPE_DATA;

	/* look up all matching names in all objects */
	for (e = htsp_first(&ctx->obj_tbl); e; e = htsp_next(&ctx->obj_tbl, e)) {
		fgw_obj_t *l = e->value;
		fgw_func_t *f = htsp_get(&l->func_tbl, func_name);
		if (f == NULL)
			continue;
		argv[0].val.argv0.func = f;
		argv[0].val.argv0.user_call_ctx = user_call_ctx;
		if (multi)
			call_func_retain(ctx, 0, argc, argv);
		else
			call_func(ctx, 0, argc, argv);
	}

	fgw_argv_free(ctx, argc, argv);
	ARG_POST;
}

void fgw_ucall_all(fgw_ctx_t *ctx, void *user_call_ctx, const char *func_name, int argc, fgw_arg_t *argv)
{
	htsp_entry_t *e;
	int multi = is_multi(ctx, func_name);

	argv[0].val.argv0.func = NULL;
	argv[0].val.argv0.user_call_ctx = user_call_ctx;
	argv[0].type = FGW_FUNC;

	/* look up all matching names in all objects */
	for (e = htsp_first(&ctx->obj_tbl); e; e = htsp_next(&ctx->obj_tbl, e)) {
		fgw_obj_t *l = e->value;
		fgw_func_t *f = htsp_get(&l->func_tbl, func_name);
		if (f == NULL)
			continue;
		argv[0].val.func = f;
		if (multi)
			call_func_retain(ctx, 0, argc, argv);
		else
			call_func(ctx, 0, argc, argv);
	}
	fgw_argv_free(ctx, argc, argv);
}

void fgw_call_all(fgw_ctx_t *ctx, const char *func_name, int argc, fgw_arg_t *argv)
{
	fgw_ucall_all(ctx, NULL, func_name, argc, argv);
}
