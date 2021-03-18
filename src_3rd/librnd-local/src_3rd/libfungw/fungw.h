/*
    fungw - language-agnostic function gateway
    Copyright (C) 2017,2019  Tibor 'Igor2' Palinkas

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

#ifndef FUNGW_H
#define FUNGW_H

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <genht/htsp.h>
#include <genht/htpp.h>

#ifdef FUNGW_CFG_LONG
#	define FGW_API_VER (0x1010 | 0x1)
#else
#	define FGW_API_VER (0x1010)
#endif
extern unsigned int fgw_api_ver;

/* maximum length of an ID (lib name or function name) */
#define FGW_ID_LEN 255

typedef enum fgw_error_e {
	FGW_SUCCESS = 0,

	FGW_ERR_ARGC,       /* wrong number of arguments */
	FGW_ERR_ARG_CONV,   /* failed to convert an argument */
	FGW_ERR_ARGV_TYPE,  /* wrong argument type */
	FGW_ERR_NOT_FOUND,
	FGW_ERR_PTR_DOMAIN, /* pointer passed from the wrong domain */

	FGW_ERR_UNKNOWN
} fgw_error_t;

typedef struct fgw_ctx_s fgw_ctx_t;
typedef struct fgw_obj_s fgw_obj_t;
typedef struct fgw_arg_s fgw_arg_t;
typedef struct fgw_eng_s fgw_eng_t;
typedef struct fgw_func_s fgw_func_t;
typedef struct fgw_custype_s fgw_custype_t;

struct fgw_ctx_s {
	htsp_t func_tbl;  /* strdup'd name to (fgw_func_t *) */
	htsp_t obj_tbl;   /* to (fgw_obj_t *) */
	htpp_t ptr_tbl;   /* void* to const-domain-string (not strdup'd) */
	void (*async_error)(fgw_obj_t *obj, const char *msg); /* called on error while executing a fungw call typicially in a binding */
	fgw_custype_t *custype; /* array of custom types, indexed as (type-FGW_CUSTOM); NULL if there's no custom type */
	char *name;
};

struct fgw_obj_s {
	char *name;
	int name_len;
	unsigned long int script_type;
	htsp_t func_tbl;  /* func->name to (fgw_func_t *)  (func name is not strup'd) */
	void *script_data, *script_user_call_ctx;
	fgw_ctx_t *parent;
	const fgw_eng_t *engine;
};

extern htsp_t fgw_engines;

struct fgw_eng_s {
	char *name;
	fgw_error_t (*call_script)(fgw_arg_t *res, int argc, fgw_arg_t *argv); /* call a function defined in the script */
	int (*init)(fgw_obj_t *obj, const char *filename, const char *opts); /* initialize the interpreter */
	int (*load)(fgw_obj_t *obj, const char *filename, const char *opts); /* load a script (external functions will be registered already) */
	int (*unload)(fgw_obj_t *obj); /* unload a script, lib is free'd by the caller */
	void (*reg_func)(fgw_obj_t *obj, const char *name, fgw_func_t *f);
	void (*unreg_func)(fgw_obj_t *obj, const char *name);
	int (*test_parse)(const char *filename, FILE *f); /* returns 1 if the file looks like something the engine can handle; if f is not NULL, it's a read-only file seeked to the beginning */
	const char *def_ext; /* default script extension */
};


typedef enum fgw_type_e {
	FGW_INVALID = 0,
	FGW_TERM = FGW_INVALID,

	FGW_CHAR = 0x0010,
	FGW_UCHAR,
	FGW_SCHAR,
	FGW_SHORT,
	FGW_USHORT,
	FGW_INT,
	FGW_UINT,
	FGW_LONG,
	FGW_ULONG,
	FGW_AUTO, /* for "custom to any" conversion -> should convert to any base (non-custom) type */

	FGW_SIZE_T = 0x0030,
#ifdef FUNGW_CFG_LONG
	FGW_LLONG,
	FGW_ULLONG,
#endif

	FGW_FLOAT = 0x0040,
	FGW_DOUBLE,
#ifdef FUNGW_CFG_LONG
	FGW_LDOUBLE,
#endif

	FGW_STRUCT = 0x0050, /* valid only wth FGW_PTR */
	FGW_VOID,            /* valid only wth FGW_PTR */
	FGW_FUNC,

	FGW_CUSTOM = 0x0060, /* the first custom type; anything between this and FGW_IN is considered a custom type */

	FGW_PTR   = 0x0400,  /* pointer to the given type (or array) */
	FGW_ZTERM = 0x0800,  /* zero-terminated array */

	FGW_DYN   = 0x1000,  /* dynamic allocated by wrappers, should be free'd */

	/* shorthands */
	FGW_STR = FGW_CHAR | FGW_PTR | FGW_ZTERM
} fgw_type_t;

#define FGW_NUM_CUSTOM_TYPES  (FGW_PTR - FGW_CUSTOM)
#define FGW_BASE_TYPE(t) ((t) & 0xFFF)
#define FGW_IS_TYPE_CUSTOM(t) ((((t) & 0x3FF) >= FGW_CUSTOM) && (((t) & 0x3FF) <= FGW_CUSTOM + FGW_NUM_CUSTOM_TYPES))

/* fungw functions have the following prototype, bundled with a binding-specific ID: */
struct fgw_func_s {
	fgw_error_t (*func)(fgw_arg_t *res, int argc, fgw_arg_t *argv);
	char *name;
	fgw_obj_t *obj;
	void *obj_data;
	void *reg_data; /* optional field to be loaded by the registrar, after function registration; e.g. for holding description or other metadata on why the function is registered */
};

typedef union fgw_value_e {
	/* nativ types */
	char               nat_char;
	unsigned char      nat_uchar;
	signed char        nat_schar;
	short              nat_short;
	unsigned short     nat_ushort;
	int                nat_int;
	unsigned int       nat_uint;
	long               nat_long;
	unsigned long      nat_ulong;
#ifdef FUNGW_CFG_LONG
	long long          nat_llong;
	unsigned long long nat_ullong;
#endif
	size_t             nat_size_t;
	float              nat_float;
	double             nat_double;
#ifdef FUNGW_CFG_LONG
	long double        nat_ldouble;
#endif

	/* pointer types */
	char               *ptr_char;
	unsigned char      *ptr_uchar;
	signed char        *ptr_schar;
	short              *ptr_short;
	unsigned short     *ptr_ushort;
	int                *ptr_int;
	unsigned int       *ptr_uint;
	long               *ptr_long;
	unsigned long      *ptr_ulong;
#ifdef FUNGW_CFG_LONG
	long long          *ptr_llong;
	unsigned long long *ptr_ullong;
#endif
	size_t             *ptr_size_t;
	float              *ptr_float;
	double             *ptr_double;
#ifdef FUNGW_CFG_LONG
	long double        *ptr_ldouble;
#endif
	void               *ptr_struct;
	void               *ptr_void;

	char               *str;
	const char         *cstr;
	fgw_func_t         *func;

	struct { fgw_func_t *func; void *user_call_ctx; }
	                   argv0; /* function call with user context also encoded in argv[0] */

	union { char c[sizeof(void *)*2]; void *p[2]; }
	                   custom;
} fgw_value_t;

struct fgw_arg_s {
	fgw_type_t type;
	fgw_value_t val;
};

/* Custom type: should use the ->custom field */
struct fgw_custype_s {
	char *name; /* slot is in use if not NULL */
	int (*arg_conv)(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target); /* converts to target or if that is not possible, to any non-custom type for a 2-stage conversion */
	int (*arg_free)(fgw_ctx_t *ctx, fgw_arg_t *arg); /* free extra memory allocated for the type for storing the value (does not free arg itself) */
};

/* call this before the applicaton exits, to clean up persistent engine data */
void fgw_atexit(void);


/*** Context handling ***/
void fgw_init(fgw_ctx_t *ctx, const char *name);
void fgw_uninit(fgw_ctx_t *ctx);

/*** object table: registration and lookup ***/
#define fgw_obj_lookup(ctx, name) ((fgw_obj_t *)htsp_get(&(ctx)->obj_tbl, (name)))
fgw_obj_t *fgw_obj_reg(fgw_ctx_t *ctx, const char *name);
void fgw_obj_unreg(fgw_ctx_t *ctx, fgw_obj_t *obj);

/* Create a new instance of an engine */
fgw_obj_t *fgw_obj_new(fgw_ctx_t *ctx, const char *obj_name, const char *eng_name, const char *filename, const char *opts);
fgw_obj_t *fgw_obj_new2(fgw_ctx_t *ctx, const char *obj_name, const char *eng_name, const char *filename, const char *opts, void *user_call_ctx);


/*** function table: registration and lookup ***/

/* Return the function for a name on NULL if not found*/
#define fgw_func_lookup_in(obj, name) ((fgw_func_t *)htsp_get(&(obj)->func_tbl, (name)))
#define fgw_func_lookup(ctx, name) ((fgw_func_t *)htsp_get(&(ctx)->func_tbl, (name)))

/* Register a named function, retruns 0 on success; re-registrtion is error */
fgw_func_t *fgw_func_reg(fgw_obj_t *obj, const char *name, fgw_error_t (*func)(fgw_arg_t *res, int argc, fgw_arg_t *argv));

/* Unregister a named function, retruns 0 if it could remove the name */
int fgw_func_unreg(fgw_obj_t *obj, const char *name);


/*** converter ***/
int fgw_arg_conv(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target_type);

void fgw_arg_free(fgw_ctx_t *ctx, fgw_arg_t *arg);
void fgw_argv_free(fgw_ctx_t *ctx, int argc, fgw_arg_t *argv);

/*** wrapper imnplementation helpers */
#define FGW_DECL_CTX \
	fgw_ctx_t *ctx = argv[0].val.func->obj->parent

#define FGW_ARGC_REQ_MATCH(expected) \
	do { if ((argc-1) != expected) return FGW_ERR_ARGC; } while(0)

#define FGW_ARGC_REQ_RANGE(minarg, maxarg) \
	do { if (((argc-1) < minarg) || ((argc-1) > maxarg)) return FGW_ERR_ARGC; } while(0)

#define FGW_ARG_CONV(arg, target_type) \
	do { if (fgw_arg_conv(ctx, arg, target_type) != 0) return FGW_ERR_ARG_CONV; } while(0)

/*** pointers ***/

void fgw_ptr_reg(fgw_ctx_t *ctx, fgw_arg_t *res, const char *ptr_domain, fgw_type_t ptr_type, void *ptr);
void fgw_ptr_unreg(fgw_ctx_t *ctx, fgw_arg_t *res, const char *ptr_domain);

/* returns non-zero if ptr is a pointer and is in the ptr_domain */
int fgw_ptr_in_domain(fgw_ctx_t *ctx, fgw_arg_t *ptr, const char *ptr_domain);

/*** debug ***/
void fgw_dump_ctx(fgw_ctx_t *ctx, FILE *f, const char *prefix);

/*** engines ***/
void fgw_eng_reg(const fgw_eng_t *eng);
void fgw_eng_unreg(const char *name);
#define fgw_eng_lookup(name) \
	((fgw_engines.table == NULL) ? ((fgw_eng_t *)NULL) : ((fgw_eng_t *)htsp_get(&fgw_engines, name)))


/*** function calls ***/

/* The ones prefixed with 'u' also pass user_call_ctx in argv0 (packed after
   the func field in the argv[0].val.argv0) */

/* Call a function by name, with const char string arguments terminated
   with NULL; return a newly allocated string with the return value. Func
   name is either a full name (obj.func) or a short name (func) */
char *fgw_scall(fgw_ctx_t *ctx, const char *func_name, ...);
char *fgw_uscall(fgw_ctx_t *ctx, void *user_call_ctx, const char *func_name, ...);

/* Call func_name (always short name) in each object that has it; objects
   are visited in random order. */
void fgw_scall_all(fgw_ctx_t *ctx, const char *func_name, ...);
void fgw_uscall_all(fgw_ctx_t *ctx, void *user_call_ctx, const char *func_name, ...);

/* Same as fgw_scall, but takes  fgw_type_t, data  pairs in vararg and
   the result is copied in res. Arg list terminated with 0. */
fgw_error_t fgw_vcall(fgw_ctx_t *ctx, fgw_arg_t *res, const char *func_name, ...);
fgw_error_t fgw_vcall_in(fgw_ctx_t *ctx, fgw_arg_t *res, const char *obj_name, const char *func_name, ...);
fgw_error_t fgw_uvcall(fgw_ctx_t *ctx, void *user_call_ctx, fgw_arg_t *res, const char *func_name, ...);
fgw_error_t fgw_uvcall_in(fgw_ctx_t *ctx, void *user_call_ctx, fgw_arg_t *res, const char *obj_name, const char *func_name, ...);

/* Same as fgw_scall_all, but takes  fgw_type_t, data  pairs in vararg */
void fgw_vcall_all(fgw_ctx_t *ctx, const char *func_name, ...);
void fgw_uvcall_all(fgw_ctx_t *ctx, void *user_call_ctx, const char *func_name, ...);

/* Same as fgw_scall_all, but takes argc/argv; caller should allocate but leave
   argv[0] empty - it is going to be overwritten */
void fgw_call_all(fgw_ctx_t *ctx, const char *func_name, int argc, fgw_arg_t *argv);
void fgw_ucall_all(fgw_ctx_t *ctx, void *user_call_ctx, const char *func_name, int argc, fgw_arg_t *argv);

/*** misc ***/
#define FGW_CFG_PUPDIR fgw_cfg_pupdir
extern const char *fgw_cfg_pupdir;

char *fgw_strdup(const char *s);

/* Throw an async error from a binding */
void fgw_async_error(fgw_obj_t *obj, const char *msg);

/* Register a new type; if id is non-zero, attempt to use that specific ID
   for the type (may fail if it's already in use), else allocate one
   dynamically. */
fgw_type_t fgw_reg_custom_type(fgw_ctx_t *ctx, fgw_type_t id, const char *name, int (*arg_conv)(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target), int (*arg_free)(fgw_ctx_t *ctx, fgw_arg_t *arg));

/* Remove a custom type; the id will never be reused so outstading arg values
   with the given type will not be given to the wrong new function. The
   conv function is set to NULL to generate an error. Returns 0 on success. */
int fgw_unreg_custom_type(fgw_ctx_t *ctx, fgw_type_t id);

/* Compare each ending to the end of the filename, return 1 on match (0 otherwise) */
int fgw_test_parse_fn(const char *filename, const char **endings);

/* Ask each engine whether it can load fn/f. If f is not NULL, it must be
   open for read and will be rewound. Returns NULL or the name of the
   first engine that is willing to load the file. */
const char *fgw_engine_find(const char *fn, FILE *f);

/*** script helpers ***/
#define fgws_ucc_save(obj) \
do { \
	void *__ucc_save__ = (obj)->script_user_call_ctx; \
	(obj)->script_user_call_ctx = argv[0].val.argv0.user_call_ctx;

#define fgws_ucc_restore(obj) \
	(obj)->script_user_call_ctx = __ucc_save__; \
} while(0)


#endif
