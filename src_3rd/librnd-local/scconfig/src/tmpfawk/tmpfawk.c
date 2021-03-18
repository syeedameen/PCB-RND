#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tmpfawk.h"
#include "db.h"
#include "openfiles.h"
#include "libs.h"
#include "log.h"
#include "dep.h"
#include "regex.h"

static double fawk_no_fmod(double a, double b) { report("ERROR: tmpfawk error: can't calculate modulo\n"); return -1; }

/* libfawk configuration */
typedef double fawk_num_t;
typedef long fawk_refco_t;
#define FAWK_API static
#define FAWK_DISABLE_FAWK_PRINT

#define fmod fawk_no_fmod

#include "libfawk_sc_fawk.c"

void libfawk_error(fawk_ctx_t *ctx, const char *str, const char *loc_fn, long loc_line, long loc_col)
{
	report("tmpfawk error: %s in line %s:%ld.%ld\n", str, loc_fn, loc_line, loc_col);
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
			report("ERROR: can't find %s for include\n", src->fn);
			return -1;
		}
	}
	else
		fclose(src->user_data);

	return 0;
}

typedef struct {
	openfiles_t ofl;
	FILE *fout, *default_fout;
	const char *wdir;
	unsigned persistent:1;
} scc_fawk_t;

#define PRINT_CELL(prnt, handle, cell) \
	do { \
		switch(cell->type) { \
			case FAWK_NUM:    prnt(handle, "%f", cell->data.num); break; \
			case FAWK_STR:    prnt(handle, "%s", cell->data.str->str); break; \
			case FAWK_STRNUM: prnt(handle, "%s", cell->data.str->str); break; \
			case FAWK_ARRAY:  prnt(handle, "<array>"); break; \
			case FAWK_FUNC:   prnt(handle, "<func:%s>", cell->data.func.name); break; \
			case FAWK_NIL:    break; \
			default:          prnt(handle, "<unknown-cell-type>"); return; \
		} \
	} \
	while(0)

#define PRINT_CELL2(prnt, cell) \
	do { \
		switch(cell->type) { \
			case FAWK_NUM:    prnt("%f", cell->data.num); break; \
			case FAWK_STR:    prnt("%s", cell->data.str->str); break; \
			case FAWK_STRNUM: prnt("%s", cell->data.str->str); break; \
			case FAWK_ARRAY:  prnt("<array>"); break; \
			case FAWK_FUNC:   prnt("<func:%s>", cell->data.func.name); break; \
			case FAWK_NIL:    break; \
			default:          prnt("<unknown-cell-type>"); return; \
		} \
	} \
	while(0)

static void bi_print(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
	scc_fawk_t *ud = (scc_fawk_t *)ctx->user_data;
	FILE *f = ud->fout;
	int n = 0;

	if (*fname == 'f') {
		char *fn;
		fawk_cast_to_str(ctx, FAWK_CFUNC_ARG(n));
		fn = FAWK_CFUNC_ARG(0)->data.str->str;
		f = openfile_open(&ud->ofl, fn, "w");
		if (f == NULL) {
			report("ERROR: fawk: fprint redirection to '%s' failed\n", fn);
			return;
		}
		n++;
	}

	for(; n < argc; n++) {
		fawk_cell_t *cell = FAWK_CFUNC_ARG(n);
		PRINT_CELL(fprintf, f, cell);
		fprintf(f, n == argc-1 ? "" : " ");
	}
}

static void bi_report(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
	int n = 0;

	for(; n < argc; n++) {
		fawk_cell_t *cell = FAWK_CFUNC_ARG(n);
		PRINT_CELL2(report, cell);
		report(n == argc-1 ? "\n" : " ");
	}
}

static void bi_logprint(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
	int n, ind;

	fawk_cast_to_num(ctx, FAWK_CFUNC_ARG(0));
	ind = FAWK_CFUNC_ARG(0)->data.num;
	for(n = 1; n < argc; n++) {
		fawk_cell_t *cell = FAWK_CFUNC_ARG(n);
		PRINT_CELL(logprintf, ind, cell);
		logprintf(ind, n == argc-1 ? "\n" : " ");
	}
}

#define conv_args_to_str() \
do { \
	int n; \
	for(n = 0; n < argc; n++) { \
		if (fawk_cast_to_str(ctx, FAWK_CFUNC_ARG(n)) != 0) { \
			report("ERROR: can not convert %s argument %d to string\n", fname, n); \
			return; \
		} \
	} \
} while(0) \

static void bi_redir(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
	char *path, *fn, *mode = "w";
	scc_fawk_t *ud = (scc_fawk_t *)ctx->user_data;

	fflush(ud->fout);

	if (argc > 0)
		fn = FAWK_CFUNC_ARG(0)->data.str->str;

	switch(argc) {
		case 0: ud->fout = ud->default_fout; return;  /* set redirection to default */
		case 1: mode = "w"; break;
		case 2: mode = FAWK_CFUNC_ARG(1)->data.str->str;; break;
		default:
			report("ERROR: fawk: redirection to '%s' failed\n", fn);
			return;
	}

	if (ud->wdir == NULL)
		path = strclone(fn);
	else
		path = str_concat("", ud->wdir, "/", fn, NULL);

	ud->fout = openfile_open(&ud->ofl, path, mode);
	if (ud->fout == NULL)
		report("ERROR: can not open redirection to '%s' ('%s') mode %s\n", path, fn, mode);
	free(path);
}

static void bi_match(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
	conv_args_to_str();
	re_comp(FAWK_CFUNC_ARG(1)->data.str->str);
	retval->type = FAWK_NUM;
	retval->data.num = re_exec(FAWK_CFUNC_ARG(0)->data.str->str);
}

static void bi_gsub(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
	fawk_cell_t *dst;
	char *pat, *err, *csub, *buff, *end;
	const char *start;
	const char *val;
	int score, slen, global;

	if (argc < 3) {
		report("ERROR: %f needs 3 arguments\n");
		return;
	}

	dst = FAWK_CFUNC_ARG(2);
	if (dst->type != FAWK_SYMREF) {
		report("ERROR: %s last argument must be a referece - did you forget the & prefix?\n", fname);
		return;
	}
	dst = fawk_symtab_deref(ctx, &dst->data.symref, 0, NULL);
	if (dst == NULL) {
		report("ERROR: %s referenced variable (3rd arg) does not exist\n", fname);
		return;
	}

	if ((fawk_cast_to_str(ctx, FAWK_CFUNC_ARG(0)) != 0) || (fawk_cast_to_str(ctx, FAWK_CFUNC_ARG(1)) != 0)) {
		report("ERROR: can not convert %s arguments to string\n", fname);
		return;
	}

	pat  = FAWK_CFUNC_ARG(0)->data.str->str;
	csub = FAWK_CFUNC_ARG(1)->data.str->str;
	global = (*fname == 'g');

	err = re_comp(pat);
	if (err != NULL) {
		report("ERROR: failed to compile regex in %s\n", fname);
		return;
	}

	fawk_cast_to_str(ctx, dst);
	val = dst->data.str->str;
	slen = strlen(csub);
	if (global)
		buff = malloc(strlen(val)*(slen+3)+32); /* big enough for worst case, when every letter and $ and ^ are replaced with sub */
	else
		buff = malloc(strlen(val)+slen+32);  /* only one replacement will be done */
	strcpy(buff, val);

	start = buff;
	do {
		score = re_exec(start);
		if (score == 0)
			break;
		end = buff + strlen(buff);
		if (eopat[0] - bopat[0] != slen) {
			int mlen = end - eopat[0]+1;
			if (mlen > 0)
				memmove((char *)(bopat[0] + slen), eopat[0], mlen);
		}
		memcpy((char *)bopat[0], csub, slen);
		start = bopat[0] + slen;
	} while(global);

	buff = realloc(buff, strlen(buff)+1);

	cell_free(ctx, dst);
	dst->type = FAWK_STR;
	dst->data.str = fawk_str_new_from_literal(ctx, buff, -1);
	free(buff);
}

static void bi_put(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
	if (argc != 2) {
		report("ERROR: put requires 2 arguments\n");
		return;
	}
	conv_args_to_str();
	put(FAWK_CFUNC_ARG(0)->data.str->str, FAWK_CFUNC_ARG(1)->data.str->str);
}

static void bi_get(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
	const char *val;
	if (argc != 1) {
		report("ERROR: %f requires 1 arguments\n", fname);
		return;
	}
	conv_args_to_str();
	val = get(FAWK_CFUNC_ARG(0)->data.str->str);
	if (*fname == 'g') {
		retval->type = FAWK_STR;
		retval->data.str = fawk_str_new_from_literal(ctx, val == NULL ? "" : val, -1);
	}
	else {
		retval->type = FAWK_NUM;
		retval->data.num = val != NULL;
	}
}


static void bi_require(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
	if (argc != 3) {
		report("ERROR: require() requires 3 arguments\n");
		return;
	}

	fawk_cast_to_num(ctx, FAWK_CFUNC_ARG(1));
	fawk_cast_to_num(ctx, FAWK_CFUNC_ARG(2));

	retval->type = FAWK_NUM;
	retval->data.num =require(FAWK_CFUNC_ARG(0)->data.str->str, FAWK_CFUNC_ARG(1)->data.num, FAWK_CFUNC_ARG(2)->data.num);
}

typedef struct {
	fawk_ctx_t *ctx;
	char *name;
	fawk_func_t fnc;
} fawk_finder_t;

static fawk_htpp_t fawk_finders;
static int fawk_finders_inited = 0;

int fawk_finder(const char *name, int logdepth, int fatal)
{
	fawk_cell_t res;
	fawk_finder_t *f = fawk_htpp_get(&fawk_finders, name);

	if (f == NULL) {
		report("ERROR: fawk_finder: broken registration\n");
		return -1;
	}
/*printf("FAWK FINDER %p %p (%s)\n", f, f->ctx, f->name);*/

	if (fawk_call1(f->ctx, f->fnc.name) == 0) {
		fawk_execret_t er;
		fawk_cell_t *a;

		a = fawk_push_alloc(f->ctx); a->type = FAWK_STR; a->data.str = fawk_str_new_from_literal(f->ctx, name, -1);
		a = fawk_push_alloc(f->ctx); a->type = FAWK_NUM; a->data.num = logdepth;
		a = fawk_push_alloc(f->ctx); a->type = FAWK_NUM; a->data.num = fatal;

		fawk_call2(f->ctx, 3);
		er = fawk_execute(f->ctx, -1);
		if (er != FAWK_ER_FIN) {
			report("ERROR: fawk: abnormal script termination in find for %s\n", name);
			return -1;
		}
	}

	fawk_pop(f->ctx, &res);
	fawk_cast_to_num(f->ctx, &res);
	return res.data.num;
}

static void bi_dep_add(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
	scc_fawk_t *ud = (scc_fawk_t *)ctx->user_data;
	const char *name;
	fawk_finder_t *f;

	if (!ud->persistent) {
		report("ERROR: dep_add: can not register a dependency from a non-persistent script (e.g. from templating)\n");
		return;
	}

	if (argc != 2) {
		report("ERROR: dep_add requires 2 arguments\n");
		return;
	}

	if (FAWK_CFUNC_ARG(1)->type != FAWK_FUNC) {
		report("ERROR: the second arg of dep_add must be a fawk function reference\n");
		return;
	}

	if (fawk_cast_to_str(ctx, FAWK_CFUNC_ARG(0)) != 0) {
		report("ERROR: can not convert dep_add first argument to string\n");
		return;
	}
	name = strclone(FAWK_CFUNC_ARG(0)->data.str->str);

	if (!fawk_finders_inited) {
		fawk_finders_inited = 1;
		fawk_htpp_init(&fawk_finders, strhash, strkeyeq);
	}

	f = malloc(sizeof(fawk_finder_t));
	f->ctx = ctx;
	f->name = strclone(name);
	f->fnc = FAWK_CFUNC_ARG(1)->data.func;
	fawk_htpp_set(&fawk_finders, f->name, f);
	dep_add(f->name, fawk_finder);
}

static void builtins(fawk_ctx_t *ctx)
{
	fawk_symtab_regcfunc(ctx, "fprint", bi_print);
	fawk_symtab_regcfunc(ctx, "print", bi_print);
	fawk_symtab_regcfunc(ctx, "report", bi_report);
	fawk_symtab_regcfunc(ctx, "logprint", bi_logprint);
	fawk_symtab_regcfunc(ctx, "redir", bi_redir);
	fawk_symtab_regcfunc(ctx, "match", bi_match);
	fawk_symtab_regcfunc(ctx, "sub", bi_gsub);
	fawk_symtab_regcfunc(ctx, "gsub", bi_gsub);
	fawk_symtab_regcfunc(ctx, "put", bi_put);
	fawk_symtab_regcfunc(ctx, "get", bi_get);
	fawk_symtab_regcfunc(ctx, "has", bi_get);
	fawk_symtab_regcfunc(ctx, "require", bi_require);
	fawk_symtab_regcfunc(ctx, "dep_add", bi_dep_add);
}

int tmpfawk(const char *wdir, const char *input, const char *output)
{
	fawk_ctx_t ctx;
	scc_fawk_t ud;
	int pres;

	memset(&ud, 0, sizeof(ud));
	memset(&ctx, 0, sizeof(ctx));
	ud.wdir = wdir;
	ud.fout = ud.default_fout = openfile_open(&ud.ofl, output, "w");
	if (ud.fout == NULL)
		return -1;

	fawk_init(&ctx);
	ctx.parser.get_char = getch1;
	ctx.parser.include = include1;

	ctx.parser.isp->user_data = fopen(input, "r");
	if (ctx.parser.isp->user_data == NULL) {
		report("ERROR: Can't open fawk script '%s' for read\n", input);
		return -1;
	}
	ctx.parser.isp->fn = fawk_strdup(&ctx, input);

	builtins(&ctx);
	pres = fawk_parse_fawk(&ctx);

	ctx.user_data = &ud;

	if (fawk_call1(&ctx, "main") == 0) {
		fawk_execret_t er;

		fawk_call2(&ctx, 0);
		er = fawk_execute(&ctx, -1);
		if (er != FAWK_ER_FIN) {
			report("ERROR: fawk: abnormal script termination (%s)\n", input);
			pres = -1;
		}
	}
	else {
		report("ERROR: fawk failed to call function main()\n");
		pres = -1;
	}

	fawk_uninit(&ctx);
	openfile_free(&ud.ofl);
	return pres;
}

int tmpfawk_persistent(const char *script_path)
{
	fawk_ctx_t *ctx;
	scc_fawk_t *ud;
	int pres;

	ud = calloc(sizeof(scc_fawk_t), 1);
	ctx = calloc(sizeof(fawk_ctx_t), 1);

	fawk_init(ctx);
	ctx->parser.get_char = getch1;
	ctx->parser.include = include1;

	ctx->parser.isp->user_data = fopen(script_path, "r");
	if (ctx->parser.isp->user_data == NULL) {
		report("ERROR: Can't open fawk script '%s' for read\n", script_path);
		return -1;
	}
	ctx->parser.isp->fn = fawk_strdup(ctx, script_path);

	builtins(ctx);
	pres = fawk_parse_fawk(ctx);

	ctx->user_data = ud;
	ud->persistent = 1;

	if (fawk_call1(ctx, "main") == 0) {
		fawk_execret_t er;

		fawk_call2(ctx, 0);
		er = fawk_execute(ctx, -1);
		if (er != FAWK_ER_FIN) {
			report("ERROR: fawk: abnormal script termination (%s)\n", script_path);
			pres = -1;
		}
	}
	else {
		report("ERROR: fawk failed to call function main()\n");
		pres = -1;
	}

	return pres;
}

