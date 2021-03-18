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

#include <stdlib.h>
#include <string.h>
#include "fungw.h"
#include <genht/hash.h>

unsigned int fgw_api_ver = FGW_API_VER;
htsp_t fgw_engines;

#ifdef FGW_PUPDIRNAME
const char *fgw_cfg_pupdir = FGW_PUPDIRNAME;
#else
const char *fgw_cfg_pupdir = NULL;
#endif


char *fgw_strdup(const char *s)
{
	int len = strlen(s);
	char *res;
	res = malloc(len + 1);
	memcpy(res, s, len + 1);
	return res;
}


void fgw_init(fgw_ctx_t *ctx, const char *name)
{
	htsp_init(&ctx->func_tbl, strhash, strkeyeq);
	htsp_init(&ctx->obj_tbl, strhash, strkeyeq);
	htpp_init(&ctx->ptr_tbl, ptrhash, ptrkeyeq);
	ctx->custype = NULL;
	if (name != NULL)
		ctx->name = fgw_strdup(name);
	else
		ctx->name = NULL;
}

void fgw_uninit(fgw_ctx_t *ctx)
{
	htsp_entry_t *e;
	int n;

	for (e = htsp_first(&ctx->obj_tbl); e; e = htsp_next(&ctx->obj_tbl, e))
		fgw_obj_unreg(ctx, e->value);

	htsp_uninit(&ctx->func_tbl);
	htsp_uninit(&ctx->obj_tbl);
	htpp_uninit(&ctx->ptr_tbl);
	if (ctx->custype != NULL) {
		for(n = 0; n < FGW_NUM_CUSTOM_TYPES; n++)
			free(ctx->custype[n].name);
		free(ctx->custype);
	}
	free(ctx->name);
}

static int build_path(char *out, fgw_obj_t *obj, const char *name)
{
	int ll, nl = strlen(name);
	if (nl > FGW_ID_LEN)
		return -1;
	if (obj != NULL) {
		ll = obj->name_len;
		memcpy(out, obj->name, ll);
	}
	else {
		out[0] = '*';
		ll = 1;
	}
	out[ll] = '.';
	memcpy(out+ll+1, name, nl+1);
	return 0;
}

/* Register a new global function in all engines */
static void fgw_func_reg_eng(fgw_ctx_t *ctx, const char *func_name, fgw_func_t *f, int is_short)
{
	htsp_entry_t *e;

	for (e = htsp_first(&ctx->obj_tbl); e; e = htsp_next(&ctx->obj_tbl, e)) {
		fgw_obj_t *obj = e->value;
		if (is_short && (obj == f->obj))
			continue; /* do not register the short name functions in its own object - the object should do direct call */
		if ((obj->engine != NULL) && (obj->engine->reg_func != NULL))
			obj->engine->reg_func(obj, func_name, f);
	}
}

fgw_func_t *fgw_func_reg(fgw_obj_t *obj, const char *name, fgw_error_t (*func)(fgw_arg_t *res, int argc, fgw_arg_t *argv))
{
	char path[2*FGW_ID_LEN+2];
	fgw_func_t *f;

	if (build_path(path, obj, name) != 0)
		return NULL;

	if (htsp_get(&obj->func_tbl, name) != NULL)
		return NULL;

	f = calloc(sizeof(fgw_func_t), 1);
	f->name = fgw_strdup(name);
	f->func = func;
	f->obj = obj;
	f->obj_data = NULL;

	/* local name */
	htsp_set(&obj->func_tbl, f->name, f);

	/* global path */
	if (htsp_get(&obj->func_tbl, path) == NULL) {
		htsp_set(&obj->parent->func_tbl, fgw_strdup(path), f);
		fgw_func_reg_eng(obj->parent, path, f, 0);
	}

	/* global short name */
	if (htsp_get(&obj->parent->func_tbl, f->name) == NULL) {
		htsp_set(&obj->parent->func_tbl, fgw_strdup(f->name), f);
		fgw_func_reg_eng(obj->parent, f->name, f, 1);
	}

	return f;
}

int fgw_func_unreg(fgw_obj_t *obj, const char *name)
{
	char path[2*FGW_ID_LEN+2];
	fgw_func_t *fnc = htsp_get(&obj->func_tbl, name);
	htsp_entry_t *ep;

	if (fnc == NULL)
		return -1;

	if (build_path(path, obj, name) != 0)
		return -1;

	ep = htsp_popentry(&obj->parent->func_tbl, path);
	if (ep != NULL) {
		if ((obj->engine != NULL) && (obj->engine->unreg_func != NULL))
			obj->engine->unreg_func(obj, path);
		free(ep->key);
	}

	ep = htsp_popentry(&obj->parent->func_tbl, name);
	if (ep != NULL) {
		if ((obj->engine != NULL) && (obj->engine->unreg_func != NULL) && (fnc->obj != obj)) /* unreg short names registered by other objects - self-registered ones shouldn't exist because of direct calls */
			obj->engine->unreg_func(obj, name);
	}

	/* if short global name is us, need to remove there and find a substitute */
	if (htsp_get(&obj->parent->func_tbl, name) == fnc) {
		htsp_entry_t *e;

		/* look for an alternative and register that */
		for (e = htsp_first(&obj->parent->obj_tbl); e; e = htsp_next(&obj->parent->obj_tbl, e)) {
			fgw_obj_t *l = e->value;
			fgw_func_t *f = htsp_get(&l->func_tbl, name);
			if ((f != NULL) && (f->obj != obj)) {
				htsp_set(&obj->parent->func_tbl, fgw_strdup(name), f);
				fgw_func_reg_eng(obj->parent, name, f, 1);
				break;
			}
		}
	}

	if (ep != NULL)
		free(ep->key);

	return 0;
}

fgw_obj_t *fgw_obj_reg(fgw_ctx_t *ctx, const char *obj_name)
{
	fgw_obj_t *obj;
	int nl = strlen(obj_name);

	if (nl > FGW_ID_LEN)
		return NULL;

	if (htsp_get(&ctx->obj_tbl, obj_name) != NULL)
		return NULL;

	obj = calloc(sizeof(fgw_obj_t), 1);
	obj->name = fgw_strdup(obj_name);
	obj->name_len = nl;
	obj->parent = ctx;
	htsp_init(&obj->func_tbl, strhash, strkeyeq);

	htsp_set(&ctx->obj_tbl, obj->name, obj);

	return obj;
}


void fgw_obj_unreg(fgw_ctx_t *ctx, fgw_obj_t *obj)
{
	htsp_entry_t *e;
	for (e = htsp_first(&obj->func_tbl); e; e = htsp_next(&obj->func_tbl, e)) {
		fgw_func_unreg(obj, e->key);
		free(e->key);
		free(e->value);
	}

	if ((obj->engine != NULL) && (obj->engine->unreg_func != NULL))
		for (e = htsp_first(&ctx->func_tbl); e; e = htsp_next(&ctx->func_tbl, e))
			obj->engine->unreg_func(obj, e->key);

	if ((obj->engine != NULL) && (obj->engine->unload != NULL))
		obj->engine->unload(obj);
	htsp_uninit(&obj->func_tbl);
	htsp_pop(&ctx->obj_tbl, obj->name);
	free(obj->name);
	free(obj);
}

void fgw_eng_reg(const fgw_eng_t *eng)
{
	if (fgw_engines.table == NULL)
		htsp_init(&fgw_engines, strhash, strkeyeq);
	htsp_set(&fgw_engines, eng->name, (void *)eng);
}

void fgw_eng_unreg(const char *name)
{
	htsp_pop(&fgw_engines, name);
}

fgw_obj_t *fgw_obj_new2(fgw_ctx_t *ctx, const char *obj_name, const char *eng_name, const char *filename, const char *opts, void *user_call_ctx)
{
	fgw_obj_t *obj;
	const fgw_eng_t *eng = htsp_get(&fgw_engines, eng_name);
	htsp_entry_t *e;

	if (eng == NULL)
		return NULL;

	obj = fgw_obj_reg(ctx, obj_name);
	if (obj == NULL)
		return NULL;

	obj->engine = eng;

	if ((eng->init != NULL) && (eng->init(obj, filename, opts) != 0)) {
		free(obj->name);
		free(obj);
		return NULL;
	}

	if (obj->engine->reg_func != NULL) {
		for (e = htsp_first(&ctx->func_tbl); e; e = htsp_next(&ctx->func_tbl, e)) {
			fgw_func_t *func = e->value;
			if (func->obj != obj)
				obj->engine->reg_func(obj, e->key, func);
		}
	}

	obj->script_user_call_ctx = user_call_ctx;
	if ((eng->load != NULL) && (eng->load(obj, filename, opts) != 0)) {
		fgw_obj_unreg(ctx, obj);
		return NULL;
	}

	obj->script_user_call_ctx = NULL;
	return obj;
}

fgw_obj_t *fgw_obj_new(fgw_ctx_t *ctx, const char *obj_name, const char *eng_name, const char *filename, const char *opts)
{
	return fgw_obj_new2(ctx, obj_name, eng_name, filename, opts, NULL);
}

void fgw_atexit(void)
{
	htsp_uninit(&fgw_engines);
}

void fgw_async_error(fgw_obj_t *obj, const char *msg)
{
	if ((obj != NULL) && (obj->parent != NULL) && (obj->parent->async_error != NULL))
		obj->parent->async_error(obj, msg);
}

fgw_type_t fgw_reg_custom_type(fgw_ctx_t *ctx, fgw_type_t id, const char *name, int (*arg_conv)(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target), int (*arg_free)(fgw_ctx_t *ctx, fgw_arg_t *arg))
{
	if (ctx->custype == NULL)
		ctx->custype = calloc(sizeof(fgw_custype_t), FGW_NUM_CUSTOM_TYPES);

	/* find a suitable id */
	if (id == FGW_INVALID) {
		for(id = 0; id < FGW_NUM_CUSTOM_TYPES; id++)
			if (ctx->custype[id].name == NULL)
				break;
		if (id == FGW_NUM_CUSTOM_TYPES)
			return FGW_INVALID;
	}
	else {
		if ((id < FGW_CUSTOM) || (id >= FGW_CUSTOM + FGW_NUM_CUSTOM_TYPES) || (ctx->custype[id].name != NULL))
			return FGW_INVALID;
		id -= FGW_CUSTOM;
	}
	ctx->custype[id].name = fgw_strdup(name);
	ctx->custype[id].arg_conv = arg_conv;
	ctx->custype[id].arg_free = arg_free;
	return id + FGW_CUSTOM;
}

int fgw_unreg_custom_type(fgw_ctx_t *ctx, fgw_type_t id)
{
	if ((id < FGW_CUSTOM) || (id >= FGW_CUSTOM + FGW_NUM_CUSTOM_TYPES) || (ctx->custype[id].name == NULL))
		return -1;
	free(ctx->custype[id].name);
	ctx->custype[id].name = NULL;
	ctx->custype[id].arg_conv = NULL;
	return 0;
}

int fgw_test_parse_fn(const char *filename, const char **endings)
{
	size_t len, maxlen = strlen(filename);
	const char *end = filename + maxlen;
	for(; *endings != NULL; endings++) {
		len = strlen(*endings);
		if ((len < maxlen-1) && (strcmp(*endings, end - len) == 0))
			return 1;
	}
	return 0;
}

const char *fgw_engine_find(const char *fn, FILE *f)
{
	const fgw_eng_t *eng;
	htsp_entry_t *e;

	for(e = htsp_first(&fgw_engines); e != NULL; e = htsp_next(&fgw_engines, e)) {
		eng = e->value;
		if (eng->test_parse != NULL) {
			if (f != NULL)
				rewind(f);
			if (eng->test_parse(fn, f) == 1)
				return e->key;
		}
	}
	return NULL;
}

