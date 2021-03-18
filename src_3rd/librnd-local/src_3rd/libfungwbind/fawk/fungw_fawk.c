/*
    fungw - language-agnostic function gateway
    Copyright (C) 2019  Tibor 'Igor2' Palinkas

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

#include <stdlib.h>
#include <libfungw/fungw.h>
#include <libfungw/fungw_conv.h>

typedef double fawk_num_t;
typedef long fawk_refco_t;
#define FAWK_API static
#include "libfawk_sc/libfawk_sc_all.c"

/* loader */
void libfawk_error(fawk_ctx_t *ctx, const char *str, const char *loc_fn, long loc_line, long loc_col)
{
	fgw_obj_t *obj = ctx->user_data;
	char tmp[256];

	fgw_async_error(obj, "fawk error: "); fgw_async_error(obj, str);
	fgw_async_error(obj, " at "); fgw_async_error(obj, loc_fn);
	sprintf(tmp, " %ld:%ld\n", loc_line, loc_col);
	fgw_async_error(obj, tmp);
}

static int getch1(fawk_ctx_t *ctx, fawk_src_t *src)
{
	FILE *f = ctx->parser.isp->user_data;
	return fgetc(f);
}

static int include1(fawk_ctx_t *ctx, fawk_src_t *src, int opening, fawk_src_t *from)
{
	if (opening) {
		FILE *f;
		if ((*src->fn != '/') && (from != NULL)) { /* calculate relative address from 'from' */
			int l1 = strlen(src->fn), l2 = strlen(from->fn);
			char *end, *fn = malloc(l1+l2+4);
			memcpy(fn, from->fn, l2+1);
			end = strrchr(fn, '/');
			if (end != NULL) {
				end++;
				memcpy(end, src->fn, l1+1);
				f = fopen(fn, "r");
			}
			else
				f = fopen(src->fn, "r");
			free(fn);
		}
		else
			f = fopen(src->fn, "r");
		src->user_data = f;
		if (f == NULL) {
			fprintf(stderr, "Can't find %s for include\n", src->fn);
			return -1;
		}
	}
	else
		fclose(src->user_data);

	return 0;
}

static void fgw_fawk_tocell(fgw_ctx_t *fctx, fawk_ctx_t *ctx, fawk_cell_t *dst, fgw_arg_t *arg)
{
#	define FGW_FAWK_TOCELL_NUM(lst, val)   dst->type = FAWK_NUM; dst->data.num = val; return;
#	define FGW_FAWK_TOCELL_STR(lst, val)   dst->type = FAWK_STR; dst->data.str = fawk_str_new_from_literal(ctx, (val == NULL ? "" : val), -1); return;
#	define FGW_FAWK_TOCELL_PTR(lst, val)   fgw_arg_conv(fctx, arg, FGW_STR); FGW_FAWK_TOCELL_STR(lst, val);
#	define FGW_FAWK_TOCELL_NIL(lst, val)   dst->type = FAWK_NIL; return;

	if (FGW_IS_TYPE_CUSTOM(arg->type))
		fgw_arg_conv(fctx, arg, FGW_AUTO);

	switch(FGW_BASE_TYPE(arg->type)) {
		ARG_CONV_CASE_LONG(lst, FGW_FAWK_TOCELL_NUM);
		ARG_CONV_CASE_LLONG(lst, FGW_FAWK_TOCELL_NUM);
		ARG_CONV_CASE_DOUBLE(lst, FGW_FAWK_TOCELL_NUM);
		ARG_CONV_CASE_LDOUBLE(lst, FGW_FAWK_TOCELL_NUM);
		ARG_CONV_CASE_PTR(lst, FGW_FAWK_TOCELL_PTR);
		ARG_CONV_CASE_STR(lst, FGW_FAWK_TOCELL_STR);
		ARG_CONV_CASE_CLASS(lst, FGW_FAWK_TOCELL_NIL);
		ARG_CONV_CASE_INVALID(lst, FGW_FAWK_TOCELL_NIL);
	}
	if (arg->type & FGW_PTR) {
		FGW_FAWK_TOCELL_PTR(lst, arg->val.ptr_void);
	}
	else {
		FGW_FAWK_TOCELL_NIL(lst, 0);
	}
}

/* Read the fawk stack and convert the result to an fgw arg */
static void fgw_fawk_toarg(fawk_ctx_t *ctx, fgw_arg_t *dst, fawk_cell_t *src)
{
	switch(src->type) {
		case FAWK_NUM:
		case FAWK_STRNUM:
			dst->type = FGW_DOUBLE;
			dst->val.nat_double = src->data.num;
			break;
		case FAWK_STR:
			dst->type = FGW_STR | FGW_DYN;
			dst->val.str = fgw_strdup(src->data.str->str);
			break;
		case FAWK_NIL:
		default:
			dst->type = FGW_PTR;
			dst->val.ptr_void = NULL;
			break;
	}
}

/* API: the script is calling an fgw function */
static void fgws_fawk_call_fgw(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
	fgw_obj_t *obj = ctx->user_data;
	int n;
	fgw_arg_t res, *argv, argv_static[16];
	fgw_func_t *func;

	func = fgw_func_lookup(obj->parent, fname);
	if (func == NULL)
		return;

	if ((argc + 1) > (sizeof(argv_static) / sizeof(argv_static[0])))
		argv = malloc((argc + 1) * sizeof(fgw_arg_t));
	else
		argv = argv_static;

	argv[0].type = FGW_FUNC;
	argv[0].val.argv0.func = func;
	argv[0].val.argv0.user_call_ctx = obj->script_user_call_ctx;

	for (n = 0; n < argc; n++)
		fgw_fawk_toarg(ctx, &argv[n+1], FAWK_CFUNC_ARG(n));

	/* Run command */
	res.type = FGW_PTR;
	res.val.ptr_void = NULL;
	if (func->func(&res, argc+1, argv) != 0)
		return;

	/* Free the array */
	fgw_argv_free(obj->parent, argc+1, argv);
	if (argv != argv_static)
		free(argv);

 fgw_fawk_tocell(obj->parent, ctx, retval, &res);
}

/* API: register an fgw function in the script, make the function visible/callable */
static void fgws_fawk_reg_func(fgw_obj_t *obj, const char *name, fgw_func_t *f)
{
	fawk_ctx_t *ctx = obj->script_data;
	fawk_symtab_regcfunc(ctx, name, fgws_fawk_call_fgw);
}

/* API: fgw calls a fawk function */
static fgw_error_t fgws_fawk_call_script(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	fgw_obj_t *obj = argv[0].val.func->obj;
	fawk_ctx_t *ctx = obj->script_data;
	fawk_cell_t r;
	int i;

	if (fawk_call1(ctx, argv[0].val.func->name) != 0)
		return FGW_ERR_NOT_FOUND;

	for (i = 1; i < argc; i++)
		fgw_fawk_tocell(obj->parent, ctx, fawk_push_alloc(ctx), &argv[i]);

	if (fawk_call2(ctx, argc-1) != 0)
		return FGW_ERR_ARGC;

	fgws_ucc_save(obj);
	i = fawk_execute(ctx, -1);
	fgws_ucc_restore(obj);

	if (i != 0)
		return FGW_ERR_UNKNOWN;

	fawk_pop(ctx, &r);
	fgw_fawk_toarg(ctx, res, &r);
	return FGW_SUCCESS;
}


/* API: unload the script */
static int fgws_fawk_unload(fgw_obj_t *obj)
{
	if (obj->script_data != NULL) {
		fawk_uninit(obj->script_data);
		free(obj->script_data);
	}
	obj->script_data = NULL;
	return 0;
}

/* Helper function for the script to register its functions */
static void fgws_fawk_freg(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
	fgw_obj_t *obj = ctx->user_data;
	fawk_cell_t *fr = FAWK_CFUNC_ARG(0);
	fgw_func_t *func;


	if (argc != 1) {
		fgw_async_error(obj, "fgw_func_reg: wrong number of arguments: need 1\n");
		return;
	}

	if (fr->type != FAWK_FUNC) {
		fgw_async_error(obj, "fgw_func_reg: need a function name (without quotes)\n");
		return;
	}

	func = fgw_func_reg(obj, fr->data.func.name, fgws_fawk_call_script);
	if (func == NULL) {
		fgw_async_error(obj, "fgw_func_reg: failed to register function ");
		fgw_async_error(obj, fr->data.func.name);
		fgw_async_error(obj, "\n");
		return;
	}

	retval->type = FAWK_NUM;
	retval->data.num = 0;
}

/* API: init the interpreter so that functions can be registered */
static int fgws_fawk_init(fgw_obj_t *obj, const char *filename, const char *opts)
{
	fawk_ctx_t *ctx;

	/* initialize the interpreter */
	obj->script_data = ctx = malloc(sizeof(fawk_ctx_t));
	if (ctx == NULL) {
		fgw_async_error(obj, "fgws_fawk_init: failed to allocate the script context\n");
		return -1;
	}
	fawk_init(ctx);

	/* add the fawk->fgw glue */
	fawk_symtab_regcfunc(ctx, "fgw_func_reg", fgws_fawk_freg);
	ctx->user_data = obj;
	return 0;
}

/* API: load a script into an object */
static int fgws_fawk_load_any(fgw_obj_t *obj, const char *filename, const char *opts, int (*parser)(fawk_ctx_t *ctx))
{
	fawk_ctx_t *ctx = obj->script_data;

	ctx->parser.get_char = getch1;
	ctx->parser.include = include1;

	/* Load the file */
	ctx->parser.isp->user_data = fopen(filename, "r");
	if (ctx->parser.isp->user_data == NULL) {
		fgw_async_error(obj, "fgws_fawk_load: failed to load the script\n");
		error:;
		fawk_uninit(ctx);
		obj->script_data = NULL;
		return -1;
	}
	ctx->parser.isp->fn = fawk_strdup(ctx, filename);

	if (parser(ctx) != 0) {
		fgw_async_error(obj, "fgws_fawk_load: failed to parse the script()\n");
		goto error;
	}

	if ((fawk_call1(ctx, "main") != 0) || (fawk_call2(ctx, 0) != 0) || fawk_execute(ctx, -1)) {
		fgw_async_error(obj, "fgws_fawk_load: failed to call main()\n");
		goto error;
	}

	return 0;
}

static int fgws_fawk_load_fawk(fgw_obj_t *obj, const char *filename, const char *opts)
{
	return fgws_fawk_load_any(obj, filename, opts, fawk_parse_fawk);
}

static int fgws_fawk_load_fbas(fgw_obj_t *obj, const char *filename, const char *opts)
{
	return fgws_fawk_load_any(obj, filename, opts, fawk_parse_fbas);
}

static int fgws_fawk_load_fpas(fgw_obj_t *obj, const char *filename, const char *opts)
{
	return fgws_fawk_load_any(obj, filename, opts, fawk_parse_fpas);
}


static int fgws_fawk_test_parse_fawk(const char *filename, FILE *f)
{
	const char *exts[] = {".fawk", NULL };
	return fgw_test_parse_fn(filename, exts);
}

static int fgws_fawk_test_parse_fbas(const char *filename, FILE *f)
{
	const char *exts[] = {".fbas", ".bas", NULL };
	return fgw_test_parse_fn(filename, exts);
}

static int fgws_fawk_test_parse_fpas(const char *filename, FILE *f)
{
	const char *exts[] = {".fpas", ".pas", NULL };
	return fgw_test_parse_fn(filename, exts);
}


/* API: engine registration: fawk */
static const fgw_eng_t fgw_fawk_eng = {
	"fawk",
	fgws_fawk_call_script,
	fgws_fawk_init,
	fgws_fawk_load_fawk,
	fgws_fawk_unload,
	fgws_fawk_reg_func,
	NULL,
	fgws_fawk_test_parse_fawk,
	".fawk"
};

/* API: engine registration: fbas */
static const fgw_eng_t fgw_fbas_eng = {
	"fbas",
	fgws_fawk_call_script,
	fgws_fawk_init,
	fgws_fawk_load_fbas,
	fgws_fawk_unload,
	fgws_fawk_reg_func,
	NULL,
	fgws_fawk_test_parse_fbas,
	".bas"
};

/* API: engine registration: fpas */
static const fgw_eng_t fgw_fpas_eng = {
	"fpas",
	fgws_fawk_call_script,
	fgws_fawk_init,
	fgws_fawk_load_fpas,
	fgws_fawk_unload,
	fgws_fawk_reg_func,
	NULL,
	fgws_fawk_test_parse_fpas,
	".pas"
};

int pplg_check_ver_fungw_fawk(int version_we_need)
{
	return 0;
}

int pplg_init_fungw_fawk(void)
{
	(void)fawk_sym_lookup; (void)fawk_array_resolve_c; /* suppress warning for unused vars */
	fgw_eng_reg(&fgw_fawk_eng);
	fgw_eng_reg(&fgw_fbas_eng);
	fgw_eng_reg(&fgw_fpas_eng);
	return 0;
}

void pplg_uninit_fungw_fawk(void)
{
	fgw_eng_unreg(fgw_fawk_eng.name);
	fgw_eng_unreg(fgw_fbas_eng.name);
	fgw_eng_unreg(fgw_fpas_eng.name);
}
