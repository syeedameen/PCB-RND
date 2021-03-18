
/*
libfawk - A function-only AWK dialect - compacted, single-source-file version
(for a human readable source code please visit the project page)

Copyright (c) 2017..2020 Tibor 'Igor2' Palinkas. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * 3. Neither the name of the copyright holder nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Project page: http://repo.hu/projects/libfawk
Source code: svn://repo.hu/libfawk/trunk
Contact the author: http://igor2.repo.hu/contact.html
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#ifndef fawk_malloc
#define fawk_malloc(ctx,size) malloc(size)
#define fawk_calloc(ctx,n,m) calloc(n, m)
#define fawk_realloc(ctx,ptr,size) realloc(ptr, size)
#define fawk_free(ctx,ptr) free(ptr)
#endif
#ifndef fawk_assert
#include <assert.h>
#define fawk_assert assert
#endif
#ifndef fawk_no_math
#include <math.h>
#define fawk_strtod(str) strtod(str, NULL)
#define fawk_fmod(x,y) fmod(x, y)
#define FAWK_NUM_PRINTF_FMT "%g"
#endif
#define GENHT_STATIC static
#define GENHT_INLINE 
#define HT_HAS_CONST_KEY 
typedef void *fawk_htpp_key_t;
typedef const void *fawk_htpp_const_key_t;
typedef void *fawk_htpp_value_t;
#define HT(x) fawk_htpp_ ## x
typedef struct {
 int flag;
 unsigned int hash;
 HT(key_t) key;
 HT(value_t) value;
} HT(entry_t);
typedef struct {
 unsigned int mask;
 unsigned int fill;
 unsigned int used;
 HT(entry_t) *table;
 unsigned int (*keyhash)(HT(const_key_t));
 int (*keyeq)(HT(const_key_t), HT(const_key_t));
} HT(t);
FAWK_API int HT(init)(HT(t) *ht, unsigned int (*keyhash)(HT(const_key_t)), int (*keyeq)(HT(const_key_t), HT(const_key_t)));
FAWK_API void HT(uninit)(HT(t) *ht);
FAWK_API int HT(resize)(HT(t) *ht, unsigned int hint);
FAWK_API int HT(has)(HT(t) *ht, HT(const_key_t) key);
FAWK_API HT(value_t) HT(get)(HT(t) *ht, HT(const_key_t) key);
FAWK_API void HT(set)(HT(t) *ht, HT(key_t) key, HT(value_t) value);
FAWK_API HT(entry_t) *HT(insert)(HT(t) *ht, HT(key_t) key, HT(value_t) value);
FAWK_API HT(value_t) HT(pop)(HT(t) *ht, HT(const_key_t) key);
GENHT_STATIC GENHT_INLINE int HT(isused)(const HT(entry_t) *entry) {return entry->flag > 0;}
GENHT_STATIC GENHT_INLINE int HT(isempty)(const HT(entry_t) *entry) {return entry->flag == 0;}
GENHT_STATIC GENHT_INLINE int HT(isdeleted)(const HT(entry_t) *entry) {return entry->flag < 0;}
GENHT_STATIC GENHT_INLINE HT(entry_t) *HT(first)(const HT(t) *ht)
{
 HT(entry_t) *entry = 0;
 if (ht->used)
  for (entry = ht->table; !HT(isused)(entry); entry++);
 return entry;
}
GENHT_STATIC GENHT_INLINE HT(entry_t) *HT(next)(const HT(t) *ht, HT(entry_t) *entry)
{
 while (++entry != ht->table + ht->mask + 1)
  if (HT(isused)(entry))
   return entry;
 return 0;
}
#ifndef HT_INVALID_VALUE
#define HT_INVALID_VALUE 0
#endif
#define HT_MINSIZE 8
#define HT_MAXSIZE (1U << 31)
#define JUMP(i,j) i += j++
#define JUMP_FIRST(i,j) j = 1, i += j++
static GENHT_INLINE void setused(HT(entry_t) *entry) {
 entry->flag = 1;
}
static GENHT_INLINE void setdeleted(HT(entry_t) *entry) {
 entry->flag = -1;
}
static GENHT_INLINE unsigned int entryhash(const HT(entry_t) *entry) {
 return entry->hash;
}
FAWK_API int HT(init)(HT(t) *ht, unsigned int (*keyhash)(HT(const_key_t)), int (*keyeq)(HT(const_key_t), HT(const_key_t))) {
 ht->mask = HT_MINSIZE - 1;
 ht->fill = 0;
 ht->used = 0;
 ht->table = calloc( ht->mask + 1, sizeof(HT(entry_t)));
 if (!ht->table)
  return -1;
 ht->keyhash = keyhash;
 ht->keyeq = keyeq;
 return 0;
}
FAWK_API void HT(uninit)(HT(t) *ht) {
 free( ht->table);
 ht->table = NULL;
}
static HT(entry_t) *lookup(HT(t) *ht, HT(const_key_t) key, unsigned int hash) {
 unsigned int mask = ht->mask;
 unsigned int i = hash;
 unsigned int j;
 HT(entry_t) *table = ht->table;
 HT(entry_t) *entry = table + (i & mask);
 HT(entry_t) *free_entry;
 if (HT(isempty)(entry))
  return entry;
 else if (HT(isdeleted)(entry))
  free_entry = entry;
 else if (entryhash(entry) == hash && ht->keyeq(entry->key, key))
  return entry;
 else
  free_entry = NULL;
 for (JUMP_FIRST(i, j); ; JUMP(i, j)) {
  entry = table + (i & mask);
  if (HT(isempty)(entry))
   return (free_entry == NULL) ? entry : free_entry;
  else if (HT(isdeleted)(entry)) {
   if (free_entry == NULL)
    free_entry = entry;
  } else if (entryhash(entry) == hash && ht->keyeq(entry->key, key))
   return entry;
 }
}
static HT(entry_t) *cleanlookup(HT(t) *ht, unsigned int hash) {
 unsigned int mask = ht->mask;
 unsigned int i = hash;
 unsigned int j;
 HT(entry_t) *table = ht->table;
 HT(entry_t) *entry = table + (i & mask);
 if (HT(isempty)(entry))
  return entry;
 for (JUMP_FIRST(i, j); ; JUMP(i, j)) {
  entry = table + (i & mask);
  if (HT(isempty)(entry))
   return entry;
 }
}
FAWK_API int HT(resize)(HT(t) *ht, unsigned int hint) {
 unsigned int newsize;
 unsigned int used = ht->used;
 HT(entry_t) *oldtable = ht->table;
 HT(entry_t) *entry;
 if (hint < used << 1)
  hint = used << 1;
 if (hint > HT_MAXSIZE)
  hint = HT_MAXSIZE;
 for (newsize = HT_MINSIZE; newsize < hint; newsize <<= 1);
 ht->table = calloc( newsize, sizeof(HT(entry_t)));
 if (!ht->table) {
  ht->table = oldtable;
  return -1;
 }
 ht->mask = newsize - 1;
 ht->fill = ht->used;
 for (entry = oldtable; used > 0; entry++)
  if (HT(isused)(entry)) {
   used--;
   *cleanlookup(ht, entryhash(entry)) = *entry;
  }
 free( oldtable);
 return 0;
}
FAWK_API int HT(has)(HT(t) *ht, HT(const_key_t) key) {
 HT(entry_t) *entry = lookup(ht, key, ht->keyhash(key));
 return HT(isused)(entry);
}
FAWK_API HT(value_t) HT(get)(HT(t) *ht, HT(const_key_t) key) {
 HT(entry_t) *entry = lookup(ht, key, ht->keyhash(key));
 return HT(isused)(entry) ? entry->value : HT_INVALID_VALUE;
}
static GENHT_INLINE void checkfill(HT(t) *ht) {
 if (ht->fill > ht->mask - (ht->mask >> 2) || ht->fill > ht->used << 2)
  HT(resize)(ht, ht->used << (ht->used > 1 << 16 ? 1 : 2));
}
FAWK_API HT(entry_t) *HT(insert)(HT(t) *ht, HT(key_t) key, HT(value_t) value) {
 unsigned int hash = ht->keyhash(key);
 HT(entry_t) *entry = lookup(ht, key, hash);
 if (HT(isused)(entry))
  return entry;
 if (HT(isempty)(entry))
  ht->fill++;
 ht->used++;
 entry->hash = hash;
 entry->key = key;
 entry->value = value;
 setused(entry);
 checkfill(ht);
 return NULL;
}
FAWK_API void HT(set)(HT(t) *ht, HT(key_t) key, HT(value_t) value) {
 HT(entry_t) *entry = HT(insert)(ht, key, value);
 if (entry)
  entry->value = value;
}
FAWK_API HT(value_t) HT(pop)(HT(t) *ht, HT(const_key_t) key) {
 HT(entry_t) *entry = lookup(ht, key, ht->keyhash(key));
 HT(value_t) v;
 if (!HT(isused)(entry))
  return HT_INVALID_VALUE;
 ht->used--;
 v = entry->value;
 setdeleted(entry);
 return v;
}
#undef HT_INVALID_VALUE
#undef HT
unsigned libfawk_hash_seed = 0x9e3779b9;
static int genht_strcasecmp(const char *s1, const char *s2) {
 for(; (*s1 != 0) && (*s2 != 0) && ((*s1 == *s2) || (tolower(*s1) == tolower(*s2))); s1++, s2++);
 return tolower(*s1) - tolower(*s2);}
static unsigned strhash(const void *key) {
 const unsigned char *p = key;
 unsigned h = libfawk_hash_seed;
 while (*p) h += (h << 2) + *p++;
 return h;
}
static int strkeyeq(const void *a, const void *b) { return !strcmp(a, b); }
#define FAWK_API_VER 1
typedef struct fawk_cell_s fawk_cell_t;
typedef struct fawk_ctx_s fawk_ctx_t;
typedef enum {
FAWK_NIL, FAWK_NUM, FAWK_STR, FAWK_STRNUM, FAWK_ARRAY, FAWK_FUNC, FAWK_SYMREF,   FAWK_CCALL_RET,   FAWK_SCALAR = FAWK_NIL
} fawk_celltype_t;
typedef struct {
 fawk_num_t num;
 fawk_refco_t refco;
 size_t used, alloced;
 char str[1];
} fawk_str_t;
typedef struct {
 fawk_celltype_t type;
 union {
  fawk_num_t num;
  fawk_str_t *str;
 } data;
} fawk_arridx_t;
typedef struct {
 fawk_refco_t refco;
 long uid;
 fawk_htpp_t hash;
 unsigned destroying:1;
} fawk_arr_t;
typedef void (*fawk_cfunc_t)(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval);
typedef struct {
 const char *name;
 fawk_cfunc_t cfunc;
 size_t ip;
 int numargs, numfixedargs;
} fawk_func_t;
typedef struct {
 union {
  fawk_cell_t *global;
  int local;
 } ref;
 char is_local;
 size_t idx_len;
 fawk_arridx_t *idx;
} fawk_symref_t;
struct fawk_cell_s {
 char *name;
 fawk_celltype_t type;
 union {
  fawk_num_t num;
  fawk_str_t *str;
  fawk_arr_t *arr;
  fawk_symref_t symref;
  fawk_func_t func;
 } data;
};
typedef enum {
FAWKI_MAKE_SYMREF, FAWKI_PUSH_NUM, FAWKI_PUSH_STR, FAWKI_PUSH_SYMVAL, FAWKI_PUSH_TOPVAR, FAWKI_PUSH_REL, FAWKI_PUSH_NIL, FAWKI_POP, FAWKI_POPJZ, FAWKI_POPJNZ, FAWKI_NEG, FAWKI_NOT, FAWKI_EQ, FAWKI_NEQ, FAWKI_LTEQ, FAWKI_GTEQ, FAWKI_LT, FAWKI_GT, FAWKI_IN, FAWKI_ADD, FAWKI_SUB, FAWKI_MUL, FAWKI_DIV, FAWKI_MOD, FAWKI_CONCAT, FAWKI_FORIN_FIRST, FAWKI_FORIN_NEXT, FAWKI_INCDEC, FAWKI_SET, FAWKI_CALL, FAWKI_RET, FAWKI_JMP, FAWKI_ABORT } fawk_ins_t;

typedef struct {
 enum {
FAWKC_INS, FAWKC_SYMREF, FAWKC_NUM, FAWKC_STR, FAWKC_CSTR } type;
 union {
  fawk_ins_t ins;
  fawk_symref_t *symref;
  fawk_num_t num;
  fawk_str_t *str;
  const char *cstr;
 } data;
 unsigned int line;
} fawk_code_t;
#define FAWK_STACK_PAGE_SIZE 256
#define FAWK_MAX_INCLUDE_STACK 16
typedef struct fawk_src_s {
 char *fn;
 long line, col, last_col;
 void *user_data;
} fawk_src_t;
typedef struct fawk_pkg_s {
 union { long l; fawk_num_t num; void *ptr; } data[8];
 void (*str_free_cb)(struct fawk_pkg_s *pkg, fawk_ctx_t *ctx, fawk_str_t *str);
 void (*uninit_cb)(struct fawk_pkg_s *pkg, fawk_ctx_t *ctx);
 struct fawk_pkg_s *next;
} fawk_pkg_t;
#define FAWK_PKG_CALL(ctx,func,args) { fawk_pkg_t *p, *next; for(p = ctx->pkg_head; p != NULL; p = next) { next = p->next; if (func != NULL) func args; } }
struct fawk_ctx_s {
 fawk_htpp_t symtab;
 struct {
  int (*get_char)(fawk_ctx_t *ctx, fawk_src_t *src);
  int (*include)(fawk_ctx_t *ctx, fawk_src_t *src, int opening, fawk_src_t *from);
  fawk_src_t *isp;
  fawk_src_t include_stack[FAWK_MAX_INCLUDE_STACK];
  int in_textblk, pushback;
  char *buff, *curr_func;
  size_t used, alloced;
  unsigned textblk_state, in_eof:1;
 } parser;
 struct {
  int alloced, used;
  int avail;
  fawk_cell_t **page;
 } stack;
 struct {
  size_t used, alloced;
  fawk_code_t *code;
 } code;
 struct {
  int numargs, numfixedargs, numidx, funcdef_offs;
  fawk_htpp_t *labels, *lablink;
 } compiler;
 size_t errbuff_alloced;
 char *errbuff;
 size_t ip;
 size_t sp, fp;
 long arr_uid;
 struct {
  unsigned trace:1;
  unsigned error:1;
 } exec;
 fawk_pkg_t *pkg_head;
 void *user_data;
};
typedef enum {
FAWK_ER_FIN, FAWK_ER_STEPS, FAWK_ER_ERROR } fawk_execret_t;

#define FAWK_STACK_INVALID (-1)
#define FAWK_CODE_INVALID (-1)
FAWK_API void fawk_init(fawk_ctx_t *ctx);
FAWK_API void fawk_uninit(fawk_ctx_t *ctx);
FAWK_API char *fawk_strdup(fawk_ctx_t *ctx, const char *s);
FAWK_API void fawk_dump_cell(fawk_cell_t *cell, int verbose);
FAWK_API fawk_str_t *fawk_str_new_from_literal(fawk_ctx_t *ctx, const char *s, size_t len_limit);
FAWK_API fawk_str_t *fawk_str_clone(fawk_ctx_t *ctx, fawk_str_t *src, size_t enlarge);
FAWK_API fawk_str_t *fawk_str_dup(fawk_ctx_t *ctx, fawk_str_t *src);
FAWK_API void fawk_str_free(fawk_ctx_t *ctx, fawk_str_t *src);
FAWK_API fawk_str_t *fawk_str_concat(fawk_ctx_t *ctx, fawk_str_t *s1, fawk_str_t *s2);
FAWK_API int fawk_cast_to_num(fawk_ctx_t *ctx, fawk_cell_t *cell);
FAWK_API int fawk_cast_to_str(fawk_ctx_t *ctx, fawk_cell_t *cell);
FAWK_API void fawk_array_init(fawk_ctx_t *ctx, fawk_cell_t *dst);
FAWK_API void fawk_array_free(fawk_ctx_t *ctx, fawk_cell_t *dst);
FAWK_API fawk_arridx_t *fawk_array_dump_list(fawk_ctx_t *ctx, fawk_cell_t *arrcell, size_t *out_len);
FAWK_API fawk_cell_t *fawk_array_resolve_c(fawk_ctx_t *ctx, int create, fawk_cell_t *arrcell, ...);
FAWK_API void libfawk_error(fawk_ctx_t *ctx, const char *str, const char *loc_fn, long loc_line, long loc_col);
#define LIBFAWK_ERROR(ctx,str,loc_fn,loc_line,loc_col,retval) \
 do { libfawk_error(ctx, str, loc_fn, loc_line, loc_col); return retval; } while(0)
FAWK_API void fawk_errbuff(fawk_ctx_t *ctx, size_t len);
FAWK_API void fawk_close_include(fawk_ctx_t *ctx, fawk_src_t *src);
#define FAWK_INCDEC_INC 1
#define FAWK_INCDEC_POST 2
#define FAWK_ERR ctx->errbuff
#define FAWK_ERROR(ctx,len,fmt) \
do { \
 fawk_errbuff(ctx, len); \
 if (ctx->errbuff != NULL) { sprintf fmt; libfawk_error(ctx, ctx->errbuff, "<runtime>", ctx->code.code[ctx->ip].line, 0); } \
 ctx->exec.error = 1; \
} while(0)
#define loop_pretest_jumpback() \
   size_t skip, back; \
   skip = fawk_pop_num(ctx, 1); \
   back = fawk_pop_num(ctx, 1); \
   fawkc_addi(ctx, FAWKI_JMP); \
   fawkc_addnum(ctx, back); \
   ctx->code.code[skip].data.num = FAWK_CURR_IP();
#define parse_aix_expr() \
   fawkc_addi(ctx, FAWKI_MAKE_SYMREF); \
   fawkc_addsymref(ctx, "SUBSEP", ctx->compiler.numidx, 0); \
   fawkc_addnum(ctx, 0); \
   fawkc_addi(ctx, FAWKI_PUSH_SYMVAL); \
   fawkc_addi(ctx, FAWKI_CONCAT);
#define fawk_parser_loop(yyctxtype,STYPE,lex,parse,ctx,next,done) \
 yyctxtype yyctx; \
 int res; \
 parse ## _init(&yyctx); \
 for(;;) { \
  STYPE lval; \
  res = parse(&yyctx, ctx, lex(&lval, ctx), &lval); \
  if (res != next) break; \
 } \
 return res != done;
FAWK_API fawk_cell_t *fawk_push_alloc(fawk_ctx_t *ctx);
FAWK_API size_t fawk_push_num(fawk_ctx_t *ctx, fawk_num_t num);
FAWK_API size_t fawk_push_str(fawk_ctx_t *ctx, const char *str);
FAWK_API int fawk_pop(fawk_ctx_t *ctx, fawk_cell_t *dst);
FAWK_API fawk_cell_t *fawk_peek(fawk_ctx_t *ctx, int addr);
FAWK_API fawk_num_t fawk_pop_num(fawk_ctx_t *ctx, int expect_num);
FAWK_API void fawk_cell_free(fawk_ctx_t *ctx, fawk_cell_t *cell);
FAWK_API void fawk_cell_cpy(fawk_ctx_t *ctx, fawk_cell_t *dst, const fawk_cell_t *src);
FAWK_API int fawk_call1(fawk_ctx_t *ctx, const char *funcname);
FAWK_API int fawk_call2(fawk_ctx_t *ctx, int argc);
FAWK_API fawk_execret_t fawk_execute(fawk_ctx_t *ctx, size_t steps);
FAWK_API void fawk_reset(fawk_ctx_t *ctx);
#define FAWK_CFUNC_ARG(argn) fawk_peek(ctx, -(argc-(argn)))
FAWK_API int fawk_builtin_init(fawk_ctx_t *ctx);
FAWK_API int fawk_symtab_regcfunc(fawk_ctx_t *ctx, const char *name, fawk_cfunc_t cfunc);
FAWK_API fawk_cell_t *fawk_symtab_regvar(fawk_ctx_t *ctx, const char *name, fawk_celltype_t tclass);
FAWK_API fawk_cell_t *fawk_sym_lookup(fawk_ctx_t *ctx, const char *name);
FAWK_API fawk_cell_t *fawk_symtab_deref(fawk_ctx_t *ctx, const fawk_symref_t *sr, int arr_create, fawk_cell_t **parent);
FAWK_API int fawk_symtab_regfunc(fawk_ctx_t *ctx, const char *name, size_t addr, int numargs, int numfixedargs);
#define keyconv(i,n,s,isnum) \
 if ((i)->type == FAWK_NUM) { isnum = 1; n = (i)->data.num; } \
 else if ((i)->type == FAWK_STRNUM) { isnum = 1; n = (i)->data.str->num; } \
 else if ((i)->type == FAWK_NIL) { isnum = 0; s = "\001NIL\001"; } \
 else if ((i)->type == FAWK_STR) { isnum = 0; s = (i)->data.str->str; } \
 else abort();
static unsigned int arrhash(const void *k)
{
 int isnum;
 fawk_num_t n;
 const char *s;
 keyconv((const fawk_arridx_t *)k, n, s, isnum);
 return isnum ? n : strhash(s);
}
static int arrkeyeq(const void *k1, const void *k2)
{
 fawk_num_t n1, n2;
 int isnum1 = 0, isnum2 = 0;
 const char *s1 = NULL, *s2 = NULL;
 keyconv((const fawk_arridx_t *)k1, n1, s1, isnum1);
 keyconv((const fawk_arridx_t *)k2, n2, s2, isnum2);
 if (isnum1 && isnum2) return n1 == n2;
 if (!isnum1 && !isnum2) return !strcmp(s1, s2);
 return 0;
}
FAWK_API void fawk_array_init(fawk_ctx_t *ctx, fawk_cell_t *dst)
{
 dst->data.arr = fawk_calloc(ctx, sizeof(fawk_arr_t), 1); if (dst->data.arr == NULL) { dst->type = FAWK_NIL; return; }
 dst->type = FAWK_ARRAY;
 dst->data.arr->uid = ctx->arr_uid++;
 dst->data.arr->refco = 1;
 fawk_htpp_init(&dst->data.arr->hash, arrhash, arrkeyeq);
}
FAWK_API void fawk_array_free(fawk_ctx_t *ctx, fawk_cell_t *dst)
{
 if (dst->data.arr->destroying)
  return;
 dst->data.arr->destroying = 1;
 dst->data.arr->refco--;
 if (dst->data.arr->refco == 0) {
  fawk_htpp_entry_t *e;
  for (e = fawk_htpp_first(&dst->data.arr->hash); e; e = fawk_htpp_next(&dst->data.arr->hash, e)) {
   fawk_arridx_t *idx = e->key;
   if ((idx->type == FAWK_STR) || (idx->type == FAWK_STRNUM))
    fawk_str_free(ctx, idx->data.str);
   fawk_cell_free(ctx, e->value);
   fawk_free(ctx, e->value);
   fawk_free(ctx, e->key);
  }
  fawk_htpp_uninit(&dst->data.arr->hash);
  fawk_free(ctx, dst->data.arr); dst->data.arr = NULL;
  dst->type = FAWK_NIL;
 }
 else
  dst->data.arr->destroying = 0;
}
FAWK_API fawk_arridx_t *fawk_array_dump_list(fawk_ctx_t *ctx, fawk_cell_t *arrcell, size_t *out_len)
{
 size_t len, n;
 fawk_arridx_t *list;
 fawk_htpp_entry_t *e;
 if ((arrcell == NULL) || (arrcell->type != FAWK_ARRAY)) return NULL;
 len = arrcell->data.arr->hash.used;
 list = fawk_malloc(ctx, sizeof(fawk_arridx_t) * len); if (list == NULL) return NULL;
 for (e = fawk_htpp_first(&arrcell->data.arr->hash), n = 0; e; e = fawk_htpp_next(&arrcell->data.arr->hash, e), n++) {
  fawk_arridx_t *idx = e->key;
  list[n].type = idx->type;
  if ((idx->type == FAWK_STR) || (idx->type == FAWK_STRNUM)) {
   list[n].data.str = fawk_str_dup(ctx, idx->data.str);
   if (list[n].data.str == NULL) { list[n].type = FAWK_NIL; ctx->exec.error = 1; }
  }
  else if (idx->type != FAWK_NIL) list[n].data.num = idx->data.num;
 }
 *out_len = len;
 return list;
}
FAWK_API fawk_cell_t *fawk_array_resolve_c(fawk_ctx_t *ctx, int create, fawk_cell_t *arrcell, ...)
{
 fawk_symref_t sr;
 fawk_arridx_t idx[64];
 va_list ap;
 int n;
 sr.ref.global = arrcell; sr.is_local = 0;
 sr.idx_len = 1; sr.idx = idx;
 va_start(ap, arrcell);
 for(n = 0; n < sizeof(idx)/sizeof(idx[0]); n++) {
  idx[n].type = (fawk_celltype_t)va_arg(ap, int);
  switch(idx[n].type) {
   case FAWK_NIL: va_end(ap); return fawk_symtab_deref(ctx, &sr, create, NULL);
   case FAWK_NUM: idx[n].data.num = va_arg(ap, fawk_num_t); break;
   case FAWK_STR: idx[n].data.str = fawk_str_new_from_literal(ctx, va_arg(ap, char *), -1); if (idx[n].data.str == NULL) goto err; break;
   default: goto err;
  }
 }
 err:; va_end(ap);
 return NULL;
}
static void fawk_bi_int(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
 if (argc == 1) {
  fawk_cell_cpy(ctx, retval, FAWK_CFUNC_ARG(0));
  fawk_cast_to_num(ctx, retval);
  retval->data.num = (long)retval->data.num;
 }
}
static void fawk_bi_length(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
 if (argc == 1) {
  fawk_cell_t *v = FAWK_CFUNC_ARG(0);
  switch(v->type) {
   case FAWK_STR:
   case FAWK_STRNUM:
    retval->data.num = v->data.str->used;
    break;
   case FAWK_ARRAY:
    retval->data.num = v->data.arr->hash.used;
    break;
   case FAWK_NIL:
   case FAWK_NUM:
   case FAWK_FUNC:
   case FAWK_SYMREF:
   case FAWK_CCALL_RET:
    return;
  }
  retval->type = FAWK_NUM;
 }
}
static void fawk_bi_delete(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
 int n;
 for(n = 0; n < argc; n++) {
  fawk_cell_t *arr, *item, *v = FAWK_CFUNC_ARG(n);
  if (v->type == FAWK_SYMREF) {
   if ((item = fawk_symtab_deref(ctx, &v->data.symref, 0, &arr)) == NULL)
    continue;
   if (arr != NULL)
    fawk_htpp_pop(&arr->data.arr->hash, &v->data.symref.idx[v->data.symref.idx_len-1]);
   fawk_cell_free(ctx, item);
  }
 }
}
static void fawk_bi_isarray(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
 fawk_cell_t *arr, *v = FAWK_CFUNC_ARG(0);
 retval->type = FAWK_NUM; retval->data.num = 0;
 if (v->type == FAWK_SYMREF) {
  if (fawk_symtab_deref(ctx, &v->data.symref, 0, &arr) == NULL) return;
  if (arr != NULL) retval->data.num = 1;
 } else if (v->type == FAWK_ARRAY) retval->data.num = 1;
}
static void fawk_bi_print_cell(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
#ifndef FAWK_DISABLE_FAWK_PRINT
 int n;
 for(n = 0; n < argc; n++) {
  fawk_dump_cell(FAWK_CFUNC_ARG(n), fname[10] == '_');
  printf(n == argc-1 ? "\n" : " ");
 }
#endif
}
static void fawk_bi_substr(fawk_ctx_t *ctx, const char *fname, int argc, fawk_cell_t *retval)
{
 fawk_cell_t *str = FAWK_CFUNC_ARG(0), *from = FAWK_CFUNC_ARG(1), *len, dummy;
 if ((argc != 2) && (argc != 3)) return;
 fawk_cast_to_str(ctx, str); fawk_cast_to_num(ctx, from);
 if (argc > 2) { len = FAWK_CFUNC_ARG(2); fawk_cast_to_num(ctx, len); } else { len = &dummy; len->data.num = str->data.str->used; }
 if (--from->data.num < 0) from->data.num = 0;
 if (from->data.num > str->data.str->used) from->data.num = str->data.str->used;
 retval->type = FAWK_STR; retval->data.str = fawk_str_new_from_literal(ctx, str->data.str->str + (long)from->data.num, len->data.num);
}
FAWK_API int fawk_builtin_init(fawk_ctx_t *ctx)
{
 fawk_cell_t *vs = fawk_symtab_regvar(ctx, "SUBSEP", FAWK_SCALAR), *vv = fawk_symtab_regvar(ctx, "FAWK_API_VER", FAWK_SCALAR);
 if ((vs == NULL) || (vv == NULL)) return -1;
 vs->type = FAWK_STR; vs->data.str = fawk_str_new_from_literal(ctx, "\034", -1); if (vs->data.str == NULL) return -1;
 vv->type = FAWK_NUM; vv->data.num = FAWK_API_VER;
 fawk_symtab_regcfunc(ctx, "int", fawk_bi_int);
 fawk_symtab_regcfunc(ctx, "length", fawk_bi_length);
 fawk_symtab_regcfunc(ctx, "delete", fawk_bi_delete);
 fawk_symtab_regcfunc(ctx, "isarray", fawk_bi_isarray);
 fawk_symtab_regcfunc(ctx, "fawk_print_cell", fawk_bi_print_cell);
 fawk_symtab_regcfunc(ctx, "fawk_print", fawk_bi_print_cell);
 fawk_symtab_regcfunc(ctx, "substr", fawk_bi_substr);
 return 0;
}
FAWK_API int fawk_cast_to_num(fawk_ctx_t *ctx, fawk_cell_t *cell)
{
 fawk_num_t res;
 switch(cell->type) {
  case FAWK_NUM: return 0;
  case FAWK_STRNUM:
   res = cell->data.str->num;
   break;
  case FAWK_STR:
   res = fawk_strtod(cell->data.str->str);
   break;
  case FAWK_FUNC:
   res = cell->data.func.ip;
   break;
  case FAWK_ARRAY:
   res = cell->data.arr->uid;
   break;
  case FAWK_SYMREF:
  case FAWK_CCALL_RET:
   FAWK_ERROR(ctx, 32, (FAWK_ERR, "cast-to-num: invalid type\n"));
   return -1;
  case FAWK_NIL:
   res = 0;
   break;
 }
 fawk_cell_free(ctx, cell);
 cell->type = FAWK_NUM;
 cell->data.num = res;
 return 0;
}
FAWK_API int fawk_cast_to_str(fawk_ctx_t *ctx, fawk_cell_t *cell)
{
 char buff[128];
 const char *res;
 fawk_num_t n;
 switch(cell->type) {
  case FAWK_NUM:
   sprintf(buff, FAWK_NUM_PRINTF_FMT, cell->data.num);
   n = cell->data.num;
   cell->data.str = fawk_str_new_from_literal(ctx, buff, -1);
   cell->data.str->num = n;
   cell->type = (cell->data.str == NULL) ? FAWK_NIL : FAWK_STRNUM;
   return 0;
  case FAWK_STRNUM:
  case FAWK_STR:
   return 0;
  case FAWK_FUNC:
   res = cell->data.func.name;
   break;
  case FAWK_SYMREF:
  case FAWK_ARRAY:
  case FAWK_CCALL_RET:
   FAWK_ERROR(ctx, 32, (FAWK_ERR, "cast-to-str: invalid type\n"));
   return -1;
  case FAWK_NIL:
   res = "";
   break;
 }
 cell->data.str = fawk_str_new_from_literal(ctx, res, -1);
 cell->type = (cell->data.str == NULL) ? FAWK_NIL : FAWK_STR;
 return 0;
}
static fawk_code_t *grow(fawk_ctx_t *ctx)
{
 if (ctx->code.used >= ctx->code.alloced) {
  fawk_code_t *c;
  ctx->code.alloced += 1024;
  if ((c = fawk_realloc(ctx, ctx->code.code, ctx->code.alloced * sizeof(fawk_code_t))) == NULL) { ctx->code.alloced = 0; return NULL; }
  ctx->code.code = c;
 }
 ctx->code.code[ctx->code.used].line = ctx->parser.isp->line+1;
 return &ctx->code.code[ctx->code.used++];
}
FAWK_API void fawkc_addi(fawk_ctx_t *ctx, fawk_ins_t ins)
{
 fawk_code_t *i = grow(ctx); if (i == NULL) { return; }
 i->type = FAWKC_INS;
 i->data.ins = ins;
}
FAWK_API void fawkc_addcs(fawk_ctx_t *ctx, const char *s)
{
 fawk_code_t *i = grow(ctx); if (i == NULL) { return; }
 i->type = FAWKC_CSTR;
 i->data.cstr = s;
}
FAWK_API void fawkc_adds(fawk_ctx_t *ctx, const char *s)
{
 fawk_code_t *i = grow(ctx); if (i == NULL) { return; }
 i->data.str = fawk_str_new_from_literal(ctx, s, -1);
 i->type = (i->data.str == NULL) ? FAWKC_NUM : FAWKC_STR;
}
FAWK_API void fawkc_addnum(fawk_ctx_t *ctx, fawk_num_t num)
{
 fawk_code_t *i = grow(ctx); if (i == NULL) { return; }
 i->type = FAWKC_NUM;
 i->data.num = num;
}
FAWK_API int fawkc_addsymref(fawk_ctx_t *ctx, const char *name, int isarr, int stack_from)
{
 fawk_code_t *i;
 fawk_cell_t *c;
 int n, offs;
 for(n = stack_from, offs = -ctx->fp-1+stack_from; n < ctx->fp; n++,offs++) {
  c = fawk_peek(ctx, n);
  fawk_assert(c->type == FAWK_STR);
  if (strcmp(name, c->data.str->str) == 0) {
   i = grow(ctx); if (i == NULL) { return -1; }
   i->type = FAWKC_SYMREF;
   i->data.symref = fawk_calloc(ctx, sizeof(fawk_symref_t), 1); if (i->data.symref == NULL) return -1;
   i->data.symref->is_local = 1;
   i->data.symref->ref.local = offs-1;
   return 0;
  }
 }
 if ((c = fawk_symtab_regvar(ctx, name, isarr ? FAWK_ARRAY : FAWK_SCALAR)) == NULL)
  return -1;
 i = grow(ctx); if (i == NULL) { return -1; }
 i->type = FAWKC_SYMREF;
 i->data.symref = fawk_calloc(ctx, sizeof(fawk_symref_t), 1); if (i->data.symref == NULL) return -1;
 i->data.symref->ref.global = c;
 return 0;
}
FAWK_API void fawkc_addi(fawk_ctx_t *ctx, fawk_ins_t ins);
FAWK_API void fawkc_addnum(fawk_ctx_t *ctx, fawk_num_t num);
FAWK_API void fawkc_addcs(fawk_ctx_t *ctx, const char *s);
FAWK_API void fawkc_adds(fawk_ctx_t *ctx, const char *s);
FAWK_API int fawkc_addsymref(fawk_ctx_t *ctx, const char *name, int isarr, int stack_from);
#define FAWK_PUSH_IP() fawk_push_num(ctx, ctx->code.used)
#define FAWK_CURR_IP() ctx->code.used
static void lazy_binop1(fawk_ctx_t *ctx, int is_or) { fawkc_addi(ctx, is_or ? FAWKI_POPJNZ : FAWKI_POPJZ); FAWK_PUSH_IP(); fawkc_addnum(ctx, 777); }
static void lazy_binop2(fawk_ctx_t *ctx, int is_or)
{
 size_t jmp1 = fawk_pop_num(ctx, 1);
 fawkc_addi(ctx, is_or ? FAWKI_POPJNZ : FAWKI_POPJZ);
 fawkc_addnum(ctx, FAWK_CURR_IP()+5);
 fawkc_addi(ctx, FAWKI_PUSH_NUM);
 fawkc_addnum(ctx, is_or ? 0 : 1);
 fawkc_addi(ctx, FAWKI_JMP);
 fawkc_addnum(ctx, FAWK_CURR_IP()+3);
 ctx->code.code[jmp1].data.num = FAWK_CURR_IP();
 fawkc_addi(ctx, FAWKI_PUSH_NUM);
 fawkc_addnum(ctx, is_or ? 1 : 0);
}
#define STACKA(addr) ((ctx->stack.page[(addr) / FAWK_STACK_PAGE_SIZE])[((addr) % FAWK_STACK_PAGE_SIZE)])
#define STACKR(offs) STACKA(ctx->sp + (offs))
#define NEED_STACK(entries) fawk_assert((ctx->sp-(entries)) >= ctx->fp);
#define FAWK_CAST_TO_NUM(cell) \
do { \
 if (cell->type != FAWK_NUM) \
  fawk_cast_to_num(ctx, cell); \
} while(0)
#define FAWK_CAST_TO_STR(cell) \
do { \
 if (cell->type != FAWK_STR) \
  fawk_cast_to_str(ctx, cell); \
} while(0)
static void cell_free(fawk_ctx_t *ctx, fawk_cell_t *cell)
{
 int n;
 switch(cell->type) {
  case FAWK_STR:
  case FAWK_STRNUM:
   if (cell->data.str != NULL)
    fawk_str_free(ctx, cell->data.str);
   break;
  case FAWK_ARRAY:
   fawk_array_free(ctx, cell);
   return;
  case FAWK_SYMREF:
   for(n = 0; (n < cell->data.symref.idx_len) && (cell->data.symref.idx_len != -1); n++)
    if ((cell->data.symref.idx[n].type == FAWK_STR) || (cell->data.symref.idx[n].type == FAWK_STRNUM))
     fawk_str_free(ctx, cell->data.symref.idx[n].data.str);
   fawk_free(ctx, cell->data.symref.idx);
   break;
  default: break;
 }
 cell->type = FAWK_NIL;
}
FAWK_API void fawk_cell_free(fawk_ctx_t *ctx, fawk_cell_t *cell)
{
 cell_free(ctx, cell);
}
static void cellcpy(fawk_ctx_t *ctx, fawk_cell_t *dst, const fawk_cell_t *src)
{
 cell_free(ctx, dst);
 *dst = *src;
 switch(src->type) {
  case FAWK_STR:
  case FAWK_STRNUM:
   dst->data.str = fawk_str_dup(ctx, src->data.str); if (dst->data.str == NULL) dst->type = FAWK_NIL;
   break;
  case FAWK_ARRAY:
   dst->type = FAWK_ARRAY;
   dst->data.arr = src->data.arr;
   dst->data.arr->refco++;
  default: break;
 }
}
FAWK_API void fawk_cell_cpy(fawk_ctx_t *ctx, fawk_cell_t *dst, const fawk_cell_t *src)
{
 cellcpy(ctx, dst, src);
}
static fawk_cell_t *push_alloc(fawk_ctx_t *ctx)
{
 fawk_cell_t *res;
 if (ctx->stack.avail == 0) {
  if (ctx->stack.used >= ctx->stack.alloced) {
   fawk_cell_t **pg;
   ctx->stack.alloced += 128;
   if ((pg = fawk_realloc(ctx, ctx->stack.page, sizeof(fawk_cell_t *) * ctx->stack.alloced)) == NULL) { ctx->stack.alloced = 0; ctx->exec.error = 1; return NULL; }
   ctx->stack.page = pg;
  }
  ctx->stack.page[ctx->stack.used] = fawk_malloc(ctx, sizeof(fawk_cell_t) * FAWK_STACK_PAGE_SIZE);
  if (ctx->stack.page[ctx->stack.used] == NULL) { ctx->exec.error = 1; return NULL;}
  ctx->stack.avail = FAWK_STACK_PAGE_SIZE; ctx->stack.used++;
 }
 ctx->stack.avail--;
 res = &STACKA(ctx->sp);
 res->name = NULL;
 res->type = FAWK_NIL;
 ctx->sp++;
 return res;
}
FAWK_API void fawk_reset(fawk_ctx_t *ctx)
{
 size_t n;
 for(n = 0; n < ctx->sp; n++)
  cell_free(ctx, &STACKA(n));
 ctx->ip = ctx->fp = ctx->sp = 0;
 fawk_free(ctx, ctx->errbuff);
 ctx->errbuff = NULL;
 ctx->errbuff_alloced = 0;
}
FAWK_API fawk_cell_t *fawk_push_alloc(fawk_ctx_t *ctx)
{
 return push_alloc(ctx);
}
FAWK_API size_t fawk_push_num(fawk_ctx_t *ctx, fawk_num_t num)
{
 fawk_cell_t *cell = push_alloc(ctx);
 if (cell == NULL)
  return FAWK_STACK_INVALID;
 cell->type = FAWK_NUM;
 cell->data.num = num;
 return ctx->sp-1;
}
FAWK_API size_t fawk_push_str(fawk_ctx_t *ctx, const char *str)
{
 fawk_cell_t *cell = push_alloc(ctx);
 if (cell == NULL)
  return FAWK_STACK_INVALID;
 cell->data.str = fawk_str_new_from_literal(ctx, str, -1);
 cell->type = (cell->data.str == NULL) ? FAWK_NIL : FAWK_STR;
 return ctx->sp-1;
}
#define PUSH() push_alloc(ctx)
#define POP() \
do { \
 cell_free(ctx, &STACKR(-1)); \
 ctx->sp--; \
 ctx->stack.avail++; \
} while(0)
FAWK_API fawk_num_t fawk_pop_num(fawk_ctx_t *ctx, int expect_num)
{
 fawk_cell_t *cell;
 NEED_STACK(1);
 cell = &STACKR(-1);
 if (cell->type != FAWK_NUM) {
  if (expect_num) {
   fawk_assert(cell->type == FAWK_NUM);
   POP();
   return 0;
  }
  fawk_cast_to_num(ctx, cell);
 }
 POP();
 return cell->data.num;
}
FAWK_API int fawk_pop(fawk_ctx_t *ctx, fawk_cell_t *dst)
{
 if (ctx->sp < 0)
  return -1;
 *dst = STACKR(-1);
 STACKR(-1).type = FAWK_NIL;
 POP();
 return 0;
}
FAWK_API fawk_cell_t *fawk_peek(fawk_ctx_t *ctx, int addr)
{
 return (addr >= 0) ? &STACKA(addr) : &STACKR(addr);
}
static fawk_cell_t *symtab_deref(fawk_ctx_t *ctx, const fawk_symref_t *sr, int arr_create, fawk_cell_t **parent)
{
 fawk_cell_t *base, *child;
 int n;
 base = (sr->is_local) ? &STACKA(ctx->fp + sr->ref.local) : sr->ref.global;
 fawk_assert(base != NULL);
 if (parent != NULL)
  *parent = NULL;
 if (sr->idx_len == 0)
  return base;
 for(n = 0; (n < sr->idx_len) && (sr->idx_len != -1); n++) {
  if (base->type == FAWK_NIL)
   fawk_array_init(ctx, base);
  else if (base->type != FAWK_ARRAY) {
   FAWK_ERROR(ctx, 64, (FAWK_ERR, "deref: symbol is not an array but is indexed like if it was\n"));
   return NULL;
  }
  if ((child = fawk_htpp_get(&base->data.arr->hash, &sr->idx[n])) == NULL) {
   fawk_arridx_t *idx;
   if (!arr_create)
    return NULL;
   child = fawk_malloc(ctx, sizeof(fawk_cell_t)); if (child == NULL) { return NULL; }
   child->type = FAWK_NIL;
   idx = fawk_malloc(ctx, sizeof(fawk_arridx_t)); if (idx == NULL) { free(child); return NULL; }
   idx->type = sr->idx[n].type;
   if ((sr->idx[n].type == FAWK_STR) || (sr->idx[n].type == FAWK_STRNUM)) {
    idx->data.str = fawk_str_dup(ctx, sr->idx[n].data.str);
    if (idx->data.str == NULL) { idx->type = FAWK_NIL; FAWK_ERROR(ctx, 64, (FAWK_ERR, "memory exhausted\n")); }
   }
   else
    idx->data.num = sr->idx[n].data.num;
   fawk_htpp_set(&base->data.arr->hash, idx, child);
  }
  if ((n < sr->idx_len-1) && (child->type == FAWK_NIL))
   fawk_array_init(ctx, child);
  if (parent != NULL)
   *parent = base;
  base = child;
 }
 return base;
}
FAWK_API fawk_cell_t *fawk_symtab_deref(fawk_ctx_t *ctx, const fawk_symref_t *sr, int arr_create, fawk_cell_t **parent)
{
 return symtab_deref(ctx, sr, arr_create, parent);
}
static fawk_cell_t *topvar(fawk_ctx_t *ctx, int and_pop)
{
 fawk_cell_t *cdst, *csrc, *cell;
 NEED_STACK(1);
 cell = &STACKR(-1);
 fawk_assert(cell->type == FAWK_SYMREF);
 if ((csrc = symtab_deref(ctx, &cell->data.symref, 1, NULL)) == NULL)
  return NULL;
 if (and_pop)
  POP();
 cdst = PUSH(); if (cdst == NULL) return NULL;
 cellcpy(ctx, cdst, csrc);
 return csrc;
}
static void exec_call(fawk_ctx_t *ctx, int argc)
{
 fawk_cell_t *fc, *nil, *vararg, *child, vtmp;
 fc = &STACKR(-(argc + 1));
 fawk_assert(fc->type == FAWK_SYMREF);
 if ((fc = symtab_deref(ctx, &fc->data.symref, 1, NULL)) == NULL)
  return;
 if (fc->type != FAWK_FUNC) {
  FAWK_ERROR(ctx, 64, (FAWK_ERR, "can't call: symbol is not a function\n"));
  return;
 }
 if (fc->data.func.cfunc == NULL) {
  if (fc->data.func.numfixedargs >= 0) {
   int vac = argc - fc->data.func.numfixedargs - 1;
   fawk_array_init(ctx, &vtmp);
   while(argc > fc->data.func.numfixedargs) {
    fawk_arridx_t *idx = malloc(sizeof(fawk_arridx_t)); if (idx == NULL) goto enomem;
    idx->type = FAWK_NUM; idx->data.num = vac--;
    child = fawk_malloc(ctx, sizeof(fawk_cell_t)); if (child == NULL) { enomem:; fawk_cell_free(ctx, &vtmp); return; }
    *child = STACKR(-1); ctx->sp--; ctx->stack.avail++; argc--;
    fawk_htpp_set(&vtmp.data.arr->hash, idx, child);
   }
   vararg = PUSH(); *vararg = vtmp;
  }
  else if (argc > fc->data.func.numargs) {FAWK_ERROR(ctx, 64 + strlen(fc->data.func.name), (FAWK_ERR, "Function '%s' called with more arguments than it takes\n", fc->data.func.name)); return; }
  while(argc < fc->data.func.numargs) {
   nil = PUSH(); if (nil == NULL) {FAWK_ERROR(ctx, 64, (FAWK_ERR, "memory exhausted\n")); return; }
   nil->type = FAWK_NIL;
   argc++;
  }
  fawk_push_num(ctx, ctx->fp);
  fawk_push_num(ctx, ctx->ip+1);
  ctx->fp = ctx->sp;
  ctx->ip = fc->data.func.ip-1;
 }
 else {
  fawk_cell_free(ctx, &STACKR(-(argc + 1)));
  fc->data.func.cfunc(ctx, fc->data.func.name, argc, &STACKR(-(argc + 1)));
  for(;argc > 0; argc--)
   POP();
  ctx->ip++;
 }
}
static int idx_steal_cell(fawk_arridx_t *res, fawk_cell_t *csrc, int allow_nil)
{
 res->type = csrc->type;
 switch(csrc->type) {
  case FAWK_NUM: res->data.num = csrc->data.num; return 0;
  case FAWK_STRNUM:
  case FAWK_STR:
   res->data.str = csrc->data.str;
   csrc->data.str = NULL;
   return 0;
  case FAWK_NIL: if (allow_nil) return 0;
  default: break;
 }
 return -1;
}
#define BREAK(num_param) ctx->ip += num_param; break
#define UNOP_ON_TOP(action) \
 NEED_STACK(1); \
 cell = &STACKR(-1); \
 FAWK_CAST_TO_NUM(cell); \
 action
#define BINOP(op1,op2) \
do { \
 NEED_STACK(2); \
 op1 = &STACKR(-1); \
 op2 = &STACKR(-2); \
} while(0)
#define BINOP_NUM(op1,op2) \
do { \
 BINOP(op1, op2); \
 FAWK_CAST_TO_NUM(op1); \
 FAWK_CAST_TO_NUM(op2); \
 POP(); \
} while(0)
#define BINOP_STRNUM(sres,op1,op2) \
do { \
 BINOP(op1, op2); \
 if ((i->data.ins == FAWKI_EQ) && (op1->type == FAWK_NIL) && (op2->type == FAWK_NIL)) { STACKR(-2).type = FAWK_NUM; sres = 0; } \
 else if ((i->data.ins == FAWKI_NEQ) && (((op1->type == FAWK_NIL) && (op2->type != FAWK_NIL)) || ((op2->type == FAWK_NIL) && (op1->type != FAWK_NIL)))) { STACKR(-2).type = FAWK_NUM; sres = 1; } \
 else if ((op1->type == FAWK_NIL) || (op2->type == FAWK_NIL)) { STACKR(-2).type = FAWK_NUM; sres = -2; }\
 else if ((op1->type == FAWK_STR) || (op2->type == FAWK_STR)) { \
  FAWK_CAST_TO_STR(op1); \
  FAWK_CAST_TO_STR(op2); \
  sres = strcmp(op2->data.str->str, op1->data.str->str); \
  STACKR(-2).type = FAWK_NUM; \
 } \
 else { \
  FAWK_CAST_TO_NUM(op1); \
  FAWK_CAST_TO_NUM(op2); \
  if (op2->data.num > op1->data.num) sres = 1; \
  else if (op2->data.num == op1->data.num) sres = 0; \
  else sres = -1; \
 } \
 POP(); \
} while(0)
FAWK_API fawk_execret_t fawk_execute(fawk_ctx_t *ctx, size_t steps)
{
 fawk_cell_t *cell, *csrc, *cdst;
 int n;
 for(;steps > 0; steps--) {
  fawk_code_t *i = &ctx->code.code[ctx->ip];
  if (ctx->exec.error)
   return FAWK_ER_ERROR;
  if (ctx->exec.trace)
   printf("[%ld] %x\n", (long)ctx->ip, i->data.ins);
  fawk_assert(i[0].type == FAWKC_INS);
  switch(i->data.ins) {
   case FAWKI_MAKE_SYMREF:
    {
     fawk_cell_t res;
     fawk_assert(i[1].type == FAWKC_SYMREF);
     fawk_assert(i[2].type == FAWKC_NUM);
     res.type = FAWK_SYMREF;
     res.data.symref.is_local = i[1].data.symref->is_local;
     res.data.symref.idx_len = i[2].data.num;
     if (i[1].data.symref->is_local) {
      res.name = "<local>";
      res.data.symref.ref.local = i[1].data.symref->ref.local;
      if ((res.data.symref.idx_len > 0) || (res.data.symref.idx_len == -1)) {
       fawk_cell_t *base = &STACKA(ctx->fp + res.data.symref.ref.local);
       if (base->type == FAWK_NIL) fawk_array_init(ctx, base);
      }
     }
     else {
      res.name = i[1].data.symref->ref.global->name;
      res.data.symref.ref.global = i[1].data.symref->ref.global;
     }
     if ((res.data.symref.idx_len > 0) && (res.data.symref.idx_len != -1)) {
      int n, d;
      res.data.symref.idx = fawk_malloc(ctx, sizeof(fawk_arridx_t) * res.data.symref.idx_len); if (res.data.symref.idx == NULL) return FAWK_ER_ERROR;
      for(n = 0, d = res.data.symref.idx_len-1; n < res.data.symref.idx_len; n++,d--) {
       if (idx_steal_cell(&(res.data.symref.idx[d]), &STACKR(-1), 1) != 0)
        abort();
       POP();
      }
     }
     else
      res.data.symref.idx = NULL;
     cell = PUSH(); if (cell == NULL) return FAWK_ER_ERROR;
     *cell = res;
    }
    BREAK(2);
   case FAWKI_PUSH_NUM:
    fawk_assert(i[1].type == FAWKC_NUM);
    cell = PUSH(); if (cell == NULL) return FAWK_ER_ERROR;
    cell->type = FAWK_NUM;
    cell->data.num = i[1].data.num;
    BREAK(1);
   case FAWKI_PUSH_STR:
    fawk_assert(i[1].type == FAWKC_STR);
    cell = PUSH(); if (cell == NULL) return FAWK_ER_ERROR;
    cell->data.str = fawk_str_dup(ctx, i[1].data.str);
    cell->type = (cell->data.str == NULL) ? FAWK_NIL : FAWK_STR;
    BREAK(1);
   case FAWKI_PUSH_SYMVAL:
    topvar(ctx, 1);
    BREAK(0);
   case FAWKI_PUSH_TOPVAR:
    topvar(ctx, 0);
    BREAK(0);
   case FAWKI_PUSH_REL:
    cell = PUSH(); if (cell == NULL) return FAWK_ER_ERROR;
    fawk_cell_cpy(ctx, cell, &STACKR(((int)i[1].data.num)-1));
    BREAK(1);
   case FAWKI_PUSH_NIL:
    cell = PUSH(); if (cell == NULL) return FAWK_ER_ERROR;
    cell->type = FAWK_NIL;
    BREAK(0);
   case FAWKI_SET:
    csrc = &STACKR(-1);
    cell = &STACKR(-2);
    fawk_assert(cell->type == FAWK_SYMREF);
    if ((cdst = symtab_deref(ctx, &cell->data.symref, 1, NULL)) == NULL)
     return FAWK_ER_ERROR;
    cellcpy(ctx, cdst, csrc);
    POP();
    BREAK(0);
   case FAWKI_POP:
    NEED_STACK(1);
    POP();
    BREAK(0);
   case FAWKI_POPJNZ:
    NEED_STACK(1);
    cell = &STACKR(-1);
    FAWK_CAST_TO_NUM(cell);
    if (cell->data.num != 0.0) {
     POP();
     goto exec_jump;
    }
    POP();
    BREAK(1);
   case FAWKI_POPJZ:
    NEED_STACK(1);
    cell = &STACKR(-1);
    FAWK_CAST_TO_NUM(cell);
    if (cell->data.num == 0.0) {
     POP();
     goto exec_jump;
    }
    POP();
    BREAK(1);
   case FAWKI_NEG:
    UNOP_ON_TOP(cell->data.num = -cell->data.num);
    BREAK(0);
   case FAWKI_NOT:
    UNOP_ON_TOP(cell->data.num = (cell->data.num == 0.0));
    BREAK(0);
   case FAWKI_EQ:
    BINOP_STRNUM(n, csrc, cdst);
    cdst->data.num = (n != -2) && (n == 0);
    BREAK(0);
   case FAWKI_NEQ:
    BINOP_STRNUM(n, csrc, cdst);
    cdst->data.num = (n != -2) && (n != 0);
    BREAK(0);
   case FAWKI_LTEQ:
    BINOP_STRNUM(n, csrc, cdst);
    cdst->data.num = (n != -2) && (n <= 0);
    BREAK(0);
   case FAWKI_GTEQ:
    BINOP_STRNUM(n, csrc, cdst);
    cdst->data.num = (n != -2) && (n >= 0);
    BREAK(0);
   case FAWKI_IN:
    {
     fawk_arridx_t resi;
     NEED_STACK(2);
     csrc = &STACKR(-1);
     fawk_assert(csrc->type == FAWK_SYMREF);
     csrc = fawk_symtab_deref(ctx, &csrc->data.symref, 0, &cell);
     if (csrc->type != FAWK_ARRAY) {
      FAWK_ERROR(ctx, 64, (FAWK_ERR, "in: symbol is not an array, can't interpret 'in' it\n"));
      return FAWK_ER_ERROR;
     }
     POP();
     cdst = &STACKR(-1);
     if (idx_steal_cell(&resi, cdst, 0) != 0) {
      FAWK_ERROR(ctx, 64, (FAWK_ERR, "in: invalid index type\n"));
      return FAWK_ER_ERROR;
     }
     n = fawk_htpp_has(&csrc->data.arr->hash, &resi);
     cell_free(ctx, cdst);
     cdst->type = FAWK_NUM;
     cdst->data.num = n;
    }
    BREAK(0);
   case FAWKI_LT:
    BINOP_STRNUM(n, csrc, cdst);
    cdst->data.num = (n != -2) && (n < 0);
    BREAK(0);
   case FAWKI_GT:
    BINOP_STRNUM(n, csrc, cdst);
    cdst->data.num = (n != -2) && (n > 0);
    BREAK(0);
   case FAWKI_ADD:
    BINOP_NUM(csrc, cdst);
    cdst->data.num += csrc->data.num;
    BREAK(0);
   case FAWKI_SUB:
    BINOP_NUM(csrc, cdst);
    cdst->data.num -= csrc->data.num;
    BREAK(0);
   case FAWKI_MUL:
    BINOP_NUM(csrc, cdst);
    cdst->data.num *= csrc->data.num;
    BREAK(0);
   case FAWKI_DIV:
    BINOP_NUM(csrc, cdst);
    cdst->data.num /= csrc->data.num;
    BREAK(0);
   case FAWKI_MOD:
    BINOP_NUM(csrc, cdst);
    cdst->data.num = fawk_fmod(cdst->data.num, csrc->data.num);
    BREAK(0);
   case FAWKI_INCDEC:
    {
     fawk_num_t diff;
     fawk_assert(i[1].type == FAWKC_NUM);
     diff = (((int)i[1].data.num) & FAWK_INCDEC_INC) ? +1 : -1;
     csrc = topvar(ctx, 1);
     cdst = &STACKR(-1);
     FAWK_CAST_TO_NUM(csrc);
     if (!(((int)i[1].data.num) & FAWK_INCDEC_POST)) {
      FAWK_CAST_TO_NUM(cdst);
      cdst->data.num += diff;
     }
     csrc->data.num += diff;
     BREAK(1);
    }
   case FAWKI_CONCAT:
    {
     fawk_str_t *ssrc, *sdst;
     NEED_STACK(2);
     csrc = &STACKR(-1);
     cdst = &STACKR(-2);
     FAWK_CAST_TO_STR(csrc); if ((csrc->type != FAWK_STR) && (csrc->type != FAWK_STRNUM)) return FAWK_ER_ERROR;
     FAWK_CAST_TO_STR(cdst); if ((cdst->type != FAWK_STR) && (cdst->type != FAWK_STRNUM)) return FAWK_ER_ERROR;
     ssrc = csrc->data.str;
     sdst = cdst->data.str;
     cdst->data.str = csrc->data.str = NULL;
     POP();
     POP();
     cell = PUSH(); if (cell == NULL) return FAWK_ER_ERROR;
     cell->data.str = fawk_str_concat(ctx, sdst, ssrc);
     cell->type = (cell->data.str == NULL) ? FAWK_NIL : FAWK_STR;
    }
    BREAK(0);
   case FAWKI_FORIN_FIRST:
    {
     fawk_arridx_t *list;
     size_t len;
     cdst = &STACKR(-1);
     fawk_assert(cdst->type == FAWK_SYMREF);
     csrc = symtab_deref(ctx, &cdst->data.symref, 0, NULL);
     list = fawk_array_dump_list(ctx, csrc, &len);
     if (list == NULL) { FAWK_ERROR(ctx, 64, (FAWK_ERR, "for-in: not an array\n")); return FAWK_ER_ERROR; }
     cell_free(ctx, cdst);
     cdst->type = FAWK_SYMREF;
     cdst->data.symref.is_local = 0;
     cdst->data.symref.ref.global = NULL;
     cdst->data.symref.idx_len = len;
     cdst->data.symref.idx = list;
    }
    BREAK(0);
   case FAWKI_FORIN_NEXT:
    {
     fawk_assert(i[1].type == FAWKC_NUM);
     csrc = &STACKR(-1);
     fawk_assert(csrc->type == FAWK_SYMREF);
     if ((csrc->data.symref.idx_len > 0) && (csrc->data.symref.idx_len != -1)) {
      cdst = &STACKR(-2);
      fawk_assert(cdst->type == FAWK_SYMREF);
      if ((cdst = symtab_deref(ctx, &cdst->data.symref, 1, NULL)) == NULL)
       return FAWK_ER_ERROR;
      cell_free(ctx, cdst);
      csrc->data.symref.idx_len--;
      if ((csrc->data.symref.idx[csrc->data.symref.idx_len].type == FAWK_STR) || (csrc->data.symref.idx[csrc->data.symref.idx_len].type == FAWK_STRNUM)) {
       cdst->type = FAWK_STR;
       cdst->data.str = csrc->data.symref.idx[csrc->data.symref.idx_len].data.str;
       csrc->data.symref.idx[csrc->data.symref.idx_len].data.str = NULL;
      }
      else if (csrc->data.symref.idx[csrc->data.symref.idx_len].type == FAWK_NIL) cdst->type = FAWK_NIL;
      else {
       cdst->type = FAWK_NUM;
       cdst->data.num = csrc->data.symref.idx[csrc->data.symref.idx_len].data.num;
      }
      ctx->ip = i[1].data.num - 1;
      break;
     }
     else {
      POP();
      POP();
     }
    }
    BREAK(1);
   case FAWKI_CALL:
    fawk_assert(i[1].type == FAWKC_NUM);
    exec_call(ctx, (int)i[1].data.num);
    BREAK(0);
   case FAWKI_RET:
    {
     int numargs, n;
     fawk_cell_t retval = STACKR(-1);
     ctx->sp--; ctx->stack.avail++;
     fawk_assert(i[1].type == FAWKC_NUM);
     numargs = i[1].data.num;
     while(STACKR(-1).type == FAWK_SYMREF) POP();
     cell = &STACKR(-1);
     if (cell->type != FAWK_NUM) { FAWK_ERROR(ctx, 64, (FAWK_ERR, "invalid symref; tip: array-in-array can not be created with [] in one step\n")); return FAWK_ER_ERROR; }
     ctx->ip = cell->data.num;
     POP();
     cell = &STACKR(-1);
     fawk_assert(cell->type == FAWK_NUM);
     ctx->fp = cell->data.num;
     POP();
     for(n = 0; n <= numargs; n++)
      POP();
     cell = PUSH(); if (cell == NULL) return FAWK_ER_ERROR;
     *cell = retval;
     cell = &STACKR(-2);
     if (cell->type == FAWK_CCALL_RET) {
      STACKR(-2) = STACKR(-1);
      STACKR(-1).type = FAWK_NIL;
      POP();
      return FAWK_ER_FIN;
     }
    }
    break;
   case FAWKI_JMP:
    exec_jump:;
    {
     int addr = i[1].data.num;
     fawk_assert(i[1].type == FAWKC_NUM);
     fawk_assert(addr >= 0);
     fawk_assert(addr < ctx->code.used);
     ctx->ip = addr-2;
    }
    BREAK(1);
   case FAWKI_ABORT:
    FAWK_ERROR(ctx, 64, (FAWK_ERR, "libfawk abort"));
    if ((i[1].type == FAWKC_STR) || (i[1].type == FAWKC_CSTR))
     libfawk_error(ctx, i[1].data.cstr, "<runtime>", ctx->code.code[ctx->ip].line, 0);
    return FAWK_ER_ERROR;
  }
  ctx->ip++;
 }
 return FAWK_ER_STEPS;
}
FAWK_API int fawk_call1(fawk_ctx_t *ctx, const char *funcname)
{
 fawk_cell_t *cell, *func = fawk_htpp_get(&ctx->symtab, funcname);
 if ((func == NULL) || (func->type != FAWK_FUNC))
  return -1;
 cell = PUSH(); if (cell == NULL) return FAWK_ER_ERROR;
 cell->type = FAWK_CCALL_RET;
 cell = PUSH(); if (cell == NULL) return FAWK_ER_ERROR;
 cell->type = FAWK_SYMREF;
 cell->data.symref.is_local = cell->data.symref.idx_len = 0;
 cell->data.symref.idx = NULL;
 cell->data.symref.ref.global = func;
 return 0;
}
FAWK_API int fawk_call2(fawk_ctx_t *ctx, int argc)
{
 fawk_cell_t *funcref = &STACKR(-argc-1), *func = funcref->data.symref.ref.global;
 if ((funcref->type != FAWK_SYMREF) || (func->type != FAWK_FUNC) || (func->data.func.numargs < argc)) {
  while(argc > -2) { POP(); argc--; }
  return -1;
 }
 exec_call(ctx, argc);
 ctx->ip++;
 return 0;
}
FAWK_API void fawk_close_include(fawk_ctx_t *ctx, fawk_src_t *src)
{
 if (src->fn == NULL) return;
 if (ctx->parser.include != NULL) ctx->parser.include(ctx, src, 0, NULL);
 fawk_free(ctx, src->fn); src->fn = NULL;
}
FAWK_API void fawk_init(fawk_ctx_t *ctx)
{
 memset(ctx, 0, sizeof(fawk_ctx_t));
 fawk_htpp_init(&ctx->symtab, strhash, strkeyeq);
 fawk_builtin_init(ctx);
 ctx->parser.isp = &ctx->parser.include_stack[0];
}
FAWK_API void fawk_uninit(fawk_ctx_t *ctx)
{
 fawk_src_t *src;
 size_t n;
 fawk_htpp_entry_t *e;
 fawk_reset(ctx);
 for(n = 0; n < ctx->stack.used; n++)
  fawk_free(ctx, ctx->stack.page[n]);
 fawk_free(ctx, ctx->stack.page);
 for (e = fawk_htpp_first(&ctx->symtab); e; e = fawk_htpp_next(&ctx->symtab, e)) {
  fawk_free(ctx, e->key);
  fawk_cell_free(ctx, e->value);
  fawk_free(ctx, e->value);
 }
 fawk_htpp_uninit(&ctx->symtab);
 for(n = 0; n < ctx->code.used; n++) {
  switch(ctx->code.code[n].type) {
   case FAWKC_INS: case FAWKC_NUM: case FAWKC_CSTR: break;
   case FAWKC_STR: fawk_str_free(ctx, ctx->code.code[n].data.str); break;
   case FAWKC_SYMREF:
    fawk_free(ctx, ctx->code.code[n].data.symref->idx);
    fawk_free(ctx, ctx->code.code[n].data.symref);
    break;
  }
 }
 fawk_free(ctx, ctx->code.code);
 fawk_free(ctx, ctx->parser.buff);
 for(src = ctx->parser.include_stack; src <= ctx->parser.isp; src++) fawk_close_include(ctx, src);
 FAWK_PKG_CALL(ctx, p->uninit_cb, (p, ctx)); FAWK_PKG_CALL(ctx, free, (p));
}
FAWK_API char *fawk_strdup(fawk_ctx_t *ctx, const char *s)
{
 size_t l = strlen(s);
 char *ret = fawk_malloc(ctx, l+1);
 if (ret != NULL) memcpy(ret, s, l+1);
 return ret;
}
FAWK_API void fawk_errbuff(fawk_ctx_t *ctx, size_t len)
{
 if (len > ctx->errbuff_alloced) {
  fawk_free(ctx, ctx->errbuff);
  ctx->errbuff_alloced = len;
  ctx->errbuff = fawk_malloc(ctx, len);
 }
 if (ctx->errbuff != NULL) *ctx->errbuff = '\0';
}
FAWK_API void fawk_dump_cell(fawk_cell_t *cell, int verbose)
{
 switch(cell->type) {
  case FAWK_NUM: if (verbose) printf("NUM:{"FAWK_NUM_PRINTF_FMT"}", cell->data.num); else printf(FAWK_NUM_PRINTF_FMT, cell->data.num); return;
  case FAWK_STR: if (verbose) printf("STR:{'%s' (ref=%ld, len=%ld/%ld)}", cell->data.str->str, (long)cell->data.str->refco, (long)cell->data.str->used, (long)cell->data.str->alloced); else printf("%s", cell->data.str->str); return;
  case FAWK_STRNUM: if (verbose) printf("STRNUM:{"FAWK_NUM_PRINTF_FMT" '%s' (ref=%ld, len=%ld/%ld)}", cell->data.str->num, cell->data.str->str, (long)cell->data.str->refco, (long)cell->data.str->used, (long)cell->data.str->alloced); else printf("%s", cell->data.str->str); return;
  case FAWK_ARRAY: printf("ARRAY:{uid=%ld len=%ld}", cell->data.arr->uid, (long)cell->data.arr->hash.used); return;
  case FAWK_FUNC: printf("FUNC:{%s}", cell->data.func.name); return;
  case FAWK_SYMREF: printf("SYMREF"); return;
  case FAWK_CCALL_RET: printf("CCAL_RET"); return;
  case FAWK_NIL: printf("NIL"); return;
 }
 printf("<invalid cell>");
}
static int getch(fawk_ctx_t *ctx)
{
 int ret;
 if (ctx->parser.pushback > 0) {
  ret = ctx->parser.pushback;
  ctx->parser.pushback = -1;
 }
 else
  ret = ctx->parser.get_char(ctx, ctx->parser.isp);
 if (ret == EOF) ctx->parser.in_eof = 1;
 else if (ret == '\n') { ctx->parser.isp->line++; ctx->parser.isp->last_col = ctx->parser.isp->col; ctx->parser.isp->col = 0; }
 else { ctx->parser.isp->col++; }
 return ret;
}
static void ungetch(fawk_ctx_t *ctx, int chr)
{
 fawk_assert(ctx->parser.pushback <= 0);
 ctx->parser.pushback = chr;
 if (chr == '\n') { ctx->parser.isp->line--; ctx->parser.isp->col = ctx->parser.isp->last_col; }
 ctx->parser.isp->col--;
}
#define append(chr,bad) \
do { \
 if (ctx->parser.used >= ctx->parser.alloced) { \
  char *bf; \
  ctx->parser.alloced += 256; \
  if ((bf = fawk_realloc(ctx, ctx->parser.buff, ctx->parser.alloced)) == NULL) { ctx->parser.alloced = 0; bad; } \
  ctx->parser.buff = bf; \
 } \
 ctx->parser.buff[ctx->parser.used++] = chr; \
} while(0)
#define del_last() ctx->parser.used--
static void fawk_readup(fawk_ctx_t *ctx, const char *accept)
{
 int c;
 do {
  c = getch(ctx);
  append(c, return);
 } while((c != EOF) && (strchr(accept, c) != NULL));
 ungetch(ctx, c);
 del_last();
}
static void readtil(fawk_ctx_t *ctx, const char *reject)
{
 int c;
 do {
  c = getch(ctx);
  append(c, return);
 } while((c != EOF) && (strchr(reject, c) == NULL));
 ungetch(ctx, c);
 del_last();
}
static int read_strlit(fawk_ctx_t *ctx, char term)
{
 int chr, len;
 for(len = 0;;len++) {
  chr = getch(ctx);
  switch(chr) {
   case '\\':
    chr = getch(ctx);
    switch(chr) {
     case 'n': append('\n', return 0); break;
     case 't': append('\t', return 0); break;
     case '0': append('\0', return 0); break;
     default: append(chr, return 0);
    }
    break;
   default: if ((chr == term) || (chr == EOF)) { append('\0', return 0); return len; }
   append(chr, return 0);
  }
 }
}
static int read_numeric(fawk_ctx_t *ctx, fawk_num_t *dst, int had_decimal_dot, int numtoken)
{
 int chr, chr2, had_e = 0;
 next:;
 chr = getch(ctx); append(chr, return -1);
 if (isdigit(chr)) goto next;
 if ((chr == '.') && (!had_decimal_dot)) { had_decimal_dot = 1; goto next; }
 if (((chr == 'e') || (chr == 'E')) && (!had_e)) {
  had_e = 1;
  chr = getch(ctx); append(chr, return -1);
  if (isdigit(chr)) goto next;
  if ((chr == '+') || (chr == '-')) {
   chr2 = getch(ctx);
   if (isdigit(chr2)) { append(chr2, return -1); goto next; }
   LIBFAWK_ERROR(ctx, "invalid numeric: e+ or e- must be followed by a digit", ctx->parser.isp->fn, ctx->parser.isp->line+1, ctx->parser.isp->col+1, -1);
  }
  LIBFAWK_ERROR(ctx, "invalid numeric: e must be followed by sign or digit", ctx->parser.isp->fn, ctx->parser.isp->line+1, ctx->parser.isp->col+1, -1);
 }
 ungetch(ctx, chr); del_last();
 append('\0', return -1);
 *dst = fawk_strtod(ctx->parser.buff);
 return numtoken;
}
#define case_op_ch2(opchr,chr2,eqtok) \
  case opchr: \
   nchr = getch(ctx); append(chr, return -1); \
   if (nchr == chr2) return eqtok; \
   else { ungetch(ctx, nchr); return chr; } \
   break
#define lex_include(restart_stmt) \
 if (ctx->parser.include != NULL) { \
  char *fn = ctx->parser.buff; \
  fawk_readup(ctx, " \t"); \
  readtil(ctx, "\""); chr = getch(ctx); \
  if (chr == '\"') { ctx->parser.used = 0; readtil(ctx, "\""); append('\0', return -1); getch(ctx); } \
  ctx->parser.isp++; \
  if (ctx->parser.isp == &ctx->parser.include_stack[FAWK_MAX_INCLUDE_STACK]) { \
   sprintf(tmp, "Includes nested too deep\n"); \
   LIBFAWK_ERROR(ctx, tmp, ctx->parser.isp->fn, ctx->parser.isp->line+1, ctx->parser.isp->col+1, -1); \
  } \
  ctx->parser.isp->fn = fawk_strdup(ctx, fn); if (ctx->parser.isp->fn == NULL) { return -1; } \
  ctx->parser.isp->line = ctx->parser.isp->col = 0; \
  if (ctx->parser.include(ctx, ctx->parser.isp, 1, &ctx->parser.isp[-1]) != 0) { \
   fawk_free(ctx, ctx->parser.isp->fn); ctx->parser.isp->fn = NULL; \
   return -1; \
  } \
  {restart_stmt;} \
 } \
 sprintf(tmp, "Include not supported by the host application\n"); \
 LIBFAWK_ERROR(ctx, tmp, ctx->parser.isp->fn, ctx->parser.isp->line+1, ctx->parser.isp->col+1, -1);
#define lex_got_eof(restart_stmt) \
 handle_eof:; ctx->parser.in_eof = 0; \
 fawk_close_include(ctx, ctx->parser.isp); \
 if (ctx->parser.isp == &ctx->parser.include_stack[0]) { fawk_free(ctx, ctx->parser.buff); ctx->parser.buff = NULL; ctx->parser.alloced = ctx->parser.used = 0; return EOF; } \
 ctx->parser.isp--; \
 { restart_stmt; }
#define lex_textblk_start(restart_stmt) \
 if ((nchr = getch(ctx)) != '[') { ungetch(ctx, nchr); return '['; } \
 ctx->parser.in_textblk = getch(ctx); ctx->parser.textblk_state = 0; restart_stmt;
#define lex_textblk_exec(token_str,restart_stmt,nlchar) if (ctx->parser.in_textblk) { \
  if (((chr = getch(ctx)) == ']') && (ctx->parser.textblk_state & 4)) { \
   if (getch(ctx) != ']') { LIBFAWK_ERROR(ctx, "expected a second ']' for closing text block", ctx->parser.isp->fn, ctx->parser.isp->line+1, ctx->parser.isp->col+1, -1); } \
   ctx->parser.in_textblk = 0; restart_stmt; \
  } \
  ctx->parser.textblk_state &= ~4; \
  if (chr == ctx->parser.in_textblk) { ctx->parser.textblk_state |= 4; } \
  if (ctx->parser.textblk_state & 2) { ungetch(ctx, chr); ctx->parser.textblk_state &= ~2; return '@'; } \
  textblk_closechar:; if (ctx->parser.textblk_state & 1) { \
   if (chr == ctx->parser.in_textblk) { ctx->parser.textblk_state &= ~1; ctx->parser.textblk_state |= 2; ctx->parser.textblk_state |= 4; restart_stmt; } \
   ungetch(ctx, chr); \
  } else { \
   ctx->parser.textblk_state |= (1 | 2 | 4); \
   ungetch(ctx, chr); ctx->parser.used = 0; read_strlit(ctx, ctx->parser.in_textblk); \
   lval->str = fawk_strdup(ctx, ctx->parser.buff); return (lval->str == NULL) ? -1 : token_str; \
  } \
 } \
 do { chr = getch(ctx); } while((chr == ' ') || (chr == '\t') || (chr == '\r') || (chr == nlchar)); \
 if (chr == ctx->parser.in_textblk) { ctx->parser.textblk_state |= 4; goto textblk_closechar; }
#define FAWK_STR_ALLOC(len) \
 fawk_malloc(ctx, sizeof(fawk_str_t) + (len)); if (res == NULL) return NULL; \
 res->refco = 1; \
 res->used = res->alloced = (len);
FAWK_API fawk_str_t *fawk_str_new_from_literal(fawk_ctx_t *ctx, const char *s, size_t len_limit)
{
 size_t slen = strlen(s), len = (len_limit == -1) ? slen : ((slen < len_limit) ? slen : len_limit);
 fawk_str_t *res = FAWK_STR_ALLOC(len);
 memcpy(res->str, s, len);
 res->str[len] = '\0';
 return res;
}
FAWK_API fawk_str_t *fawk_str_clone(fawk_ctx_t *ctx, fawk_str_t *src, size_t enlarge)
{
 fawk_str_t *res = FAWK_STR_ALLOC(src->used + enlarge);
 memcpy(res->str, src->str, src->used+1);
 res->num = src->num;
 return res;
}
FAWK_API fawk_str_t *fawk_str_dup(fawk_ctx_t *ctx, fawk_str_t *src)
{
 src->refco++;
 if (src->refco == 0) {
  src->refco--;
  return fawk_str_clone(ctx, src, 0);
 }
 return src;
}
FAWK_API void fawk_str_free(fawk_ctx_t *ctx, fawk_str_t *src)
{
 fawk_assert(src->refco > 0);
 src->refco--;
 if (src->refco == 0) {
  FAWK_PKG_CALL(ctx, p->str_free_cb, (p, ctx, src));
  fawk_free(ctx, src);
 }
}
FAWK_API fawk_str_t *fawk_str_concat(fawk_ctx_t *ctx, fawk_str_t *s1, fawk_str_t *s2)
{
 fawk_str_t *res = FAWK_STR_ALLOC(s1->used + s2->used);
 memcpy(res->str, s1->str, s1->used);
 memcpy(res->str+s1->used, s2->str, s2->used+1);
 fawk_str_free(ctx, s1);
 fawk_str_free(ctx, s2);
 return res;
}
FAWK_API fawk_cell_t *fawk_sym_lookup(fawk_ctx_t *ctx, const char *name)
{
 return fawk_htpp_get(&ctx->symtab, name);
}
#define FAWK_SYMTAB_REG(typ,symname,funcname,err_ret) \
 f = fawk_malloc(ctx, sizeof(fawk_cell_t)); if (f == NULL) return err_ret; \
 goto setup; setup:; \
 f->type = typ; \
 f->name = fawk_strdup(ctx, symname); if (f->name == NULL) { fawk_free(ctx, f); return err_ret; } \
 f->data.func.cfunc = NULL; \
 f->data.func.name = funcname; \
 fawk_htpp_set(&ctx->symtab, f->name, f);
FAWK_API int fawk_symtab_regcfunc(fawk_ctx_t *ctx, const char *name, fawk_cfunc_t cfunc)
{
 fawk_cell_t *f = fawk_htpp_get(&ctx->symtab, name);
 if (f != NULL)
  return -1;
 FAWK_SYMTAB_REG(FAWK_FUNC, name, f->name, -1);
 f->data.func.cfunc = cfunc;
 return 0;
}
FAWK_API int fawk_symtab_regfunc(fawk_ctx_t *ctx, const char *name, size_t addr, int numargs, int numfixedargs)
{
 fawk_cell_t *f = fawk_htpp_get(&ctx->symtab, name);
 if (f == NULL) {
  FAWK_SYMTAB_REG(FAWK_FUNC, name, f->name, -1);
  f->data.func.ip = addr;
  f->data.func.numargs = numargs;
  f->data.func.numfixedargs = numfixedargs;
 }
 else {
  if (f->type != FAWK_FUNC) {
   if (f->type == FAWK_NIL)
    goto setup;
   else
    FAWK_ERROR(ctx, 64+strlen(name), (FAWK_ERR, "funcreg: '%s' collides with another global symbol\n", name));
  }
  if ((f->data.func.ip == FAWK_CODE_INVALID) && (addr != FAWK_CODE_INVALID)) {
   f->data.func.ip = addr;
   f->data.func.numargs = numargs;
  }
 }
 return 0;
}
FAWK_API fawk_cell_t *fawk_symtab_regvar(fawk_ctx_t *ctx, const char *name, fawk_celltype_t tclass)
{
 fawk_cell_t *f = fawk_htpp_get(&ctx->symtab, name);
 fawk_assert((tclass == FAWK_SCALAR) || (tclass == FAWK_ARRAY));
 if (f == NULL) {
  FAWK_SYMTAB_REG(tclass, name, NULL, NULL);
  if (tclass == FAWK_ARRAY)
   fawk_array_init(ctx, f);
 }
 return f;
}
#ifndef _fawk__defines_h_
#define _fawk__defines_h_ 
typedef short fawk_int_t;
#define fawk_chr yyctx->chr
#define fawk_val yyctx->val
#define fawk_lval yyctx->lval
#define fawk_stack yyctx->stack
#define fawk_debug yyctx->debug
#define fawk_nerrs yyctx->nerrs
#define fawk_errflag yyctx->errflag
#define fawk_state yyctx->state
#define fawk_yyn yyctx->yyn
#define fawk_yym yyctx->yym
#define fawk_jump yyctx->jump
typedef union {
 char *str;
 fawk_num_t num;
} fawk_tokunion_t;
typedef fawk_tokunion_t fawk_STYPE;
#define T_FUNCTION 257
#define T_EQ 258
#define T_NEQ 259
#define T_GTEQ 260
#define T_LTEQ 261
#define T_PLEQ 262
#define T_MIEQ 263
#define T_MUEQ 264
#define T_DIEQ 265
#define T_MOEQ 266
#define T_NIL 267
#define T_IF 268
#define T_ELSE 269
#define T_FOR 270
#define T_IN 271
#define T_DO 272
#define T_WHILE 273
#define T_RETURN 274
#define T_ID 275
#define T_STRING 276
#define T_NUM 277
#define T_OR 278
#define T_AND 279
#define T_MM 280
#define T_PP 281
#define fawk_ERRCODE 256
#ifndef fawk_INITSTACKSIZE
#define fawk_INITSTACKSIZE 200
#endif
typedef struct {
 unsigned stacksize;
 fawk_int_t *s_base;
 fawk_int_t *s_mark;
 fawk_int_t *s_last;
 fawk_STYPE *l_base;
 fawk_STYPE *l_mark;
} fawk_STACKDATA;
typedef struct {
 int errflag;
 int chr;
 fawk_STYPE val;
 fawk_STYPE lval;
 int nerrs;
 int yym, yyn, state;
 int jump;
 int stack_max_depth;
 int debug;
 fawk_STACKDATA stack;
} fawk_yyctx_t;
typedef enum { fawk_RES_NEXT, fawk_RES_DONE, fawk_RES_ABORT } fawk_res_t;
FAWK_API int fawk_parse_init(fawk_yyctx_t *yyctx);
FAWK_API fawk_res_t fawk_parse(fawk_yyctx_t *yyctx, fawk_ctx_t *ctx, int tok, fawk_STYPE *lval);
FAWK_API void fawk_error(fawk_ctx_t *ctx, fawk_STYPE tok, const char *msg);
#endif
FAWK_API int fawk_parse_fawk(fawk_ctx_t *ctx);
#ifdef YY_QUERY_API_VER
#define YY_BYACCIC 
#define YY_API_MAJOR 1
#define YY_API_MINOR 0
#endif
#define fawk_EMPTY (-1)
#define fawk_clearin (fawk_chr = fawk_EMPTY)
#define fawk_errok (fawk_errflag = 0)
#define fawk_RECOVERING() (fawk_errflag != 0)
#define fawk_ENOMEM (-2)
#define fawk_EOF 0
static const fawk_int_t fawk_lhs[] = {-1,2,0,1,1,4,6,3,5,5,5,5,5,8,8,8,8,8,8,8,8,8,7,7,10,15,10,16,14,11,11,20,18,22,23,24,17,25,13,26,27,12,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,21,21,35,19,33,33,33,33,34,34,34,34,34,36,37,36,39,28,38,38,38,40,41,30,42,31,43,32,29,44,29,45,29,46,29,47,29,48,29,};
static const fawk_int_t fawk_len[] = {2,0,2,2,0,0,0,10,0,1,3,3,5,2,1,1,1,1,3,2,3,1,0,2,1,0,4,0,6,1,1,0,8,0,0,0,12,0,7,0,0,7,1,1,1,1,2,1,1,3,3,3,3,3,3,3,3,1,1,1,3,3,3,3,3,2,2,2,3,1,1,0,0,3,2,2,2,2,0,3,4,3,3,1,0,4,0,5,0,1,3,0,0,7,0,4,0,4,3,0,4,0,4,0,4,0,4,0,4,};
static const fawk_int_t fawk_defred[] = {1,0,0,0,2,0,5,3,0,0,9,0,0,0,6,0,10,0,11,0,22,0,0,12,44,0,0,37,39,0,72,43,42,0,0,0,0,0,0,22,7,21,0,23,0,14,15,16,17,0,29,30,0,47,48,57,58,59,69,0,0,0,0,19,0,78,0,0,0,77,76,0,0,46,0,0,0,0,0,91,94,96,0,0,0,0,0,0,0,0,86,13,25,99,101,103,105,107,0,75,74,0,0,0,0,0,0,18,0,49,20,0,0,0,0,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,27,0,33,0,0,0,0,0,0,0,0,0,26,0,0,0,0,0,0,0,0,0,40,81,82,79,0,0,92,0,87,28,31,0,0,0,84,80,0,90,0,34,38,41,0,0,32,0,0,0,35,0,36,};
static const fawk_int_t fawk_dgoto[] = {1,4,2,5,8,12,17,22,43,44,45,46,47,48,49,128,153,50,51,52,175,104,155,182,186,61,62,170,53,54,55,56,57,58,108,65,162,179,146,127,116,173,117,118,129,130,131,132,133,};
static const fawk_int_t fawk_sindex[] = {0,0,-249,-259,0,-249,0,0,-19,-44,0,13,2,18,0,-43,0,-38,0,45,0,62,-33,0,0,69,72,0,0,27,0,0,0,55,55,55,-162,-162,55,0,0,0,-162,0,430,0,0,0,0,-155,0,0,366,0,0,0,0,0,0,55,55,12,78,0,439,0,64,64,79,0,0,467,-18,0,55,55,55,55,-162,0,0,0,55,55,55,55,55,55,55,55,0,0,0,0,0,0,0,0,55,0,0,476,580,35,71,-150,55,0,-13,0,0,181,181,181,181,0,55,55,55,181,181,361,64,64,79,79,79,55,12,55,55,55,55,55,580,0,-162,0,92,504,-237,44,513,790,1039,541,93,0,580,580,580,580,580,
12,95,55,55,0,0,0,0,580,-31,0,55,0,0,0,74,550,12,0,0,55,0,12,0,0,0,55,580,0,55,580,97,0,12,0,};
static const fawk_int_t fawk_rindex[] = {0,0,139,0,0,139,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-3,0,0,112,0,0,0,0,0,0,0,81,0,0,0,0,0,812,836,121,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-35,608,0,0,0,0,84,0,0,908,931,937,963,0,0,0,0,969,995,899,863,872,153,378,402,100,0,0,0,0,0,0,-30,0,0,0,0,0,0,0,0,-40,17,109,0,0,-10,10,58,268,293,0,0,81,0,0,0,0,0,-27,0,0,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,316,0,119,-12,0,0,0,0,};
static const fawk_int_t fawk_gindex[] = {0,162,0,0,0,0,0,130,33,1229,0,0,0,0,0,0,0,0,0,37,0,-146,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,0,0,0,0,0,0,0,0,0,};
#define fawk_TABLESIZE 1411
static const fawk_int_t fawk_table[] = {35,95,11,19,95,42,70,38,3,168,33,98,34,171,98,35,6,83,95,95,42,9,38,95,70,33,41,34,98,98,24,100,85,140,100,24,184,24,158,159,24,41,24,14,8,35,15,8,100,100,42,102,38,95,102,33,24,34,97,13,35,97,172,98,16,42,83,38,102,102,33,41,34,69,70,97,97,35,141,73,97,85,42,100,38,20,63,33,35,34,39,21,40,42,105,38,98,103,33,104,34,89,104,102,90,39,87,110,23,59,97,88,60,30,92,115,104,104,106,90,24,73,24,138,73,73,73,73,73,73,137,73,156,176,165,39,167,160,185,4,71,88,
73,73,73,73,73,73,73,45,89,104,45,45,45,45,45,45,67,45,71,147,67,67,67,67,67,7,67,72,45,45,45,154,45,45,45,73,174,67,67,67,0,67,67,67,166,0,0,0,62,0,0,0,62,62,62,62,62,0,62,0,0,178,0,45,0,0,181,0,0,62,62,62,67,62,62,62,89,187,0,90,0,87,85,0,86,0,88,0,0,10,18,0,24,25,0,26,95,27,28,29,30,31,32,84,62,36,37,24,25,0,26,0,27,28,29,30,31,32,0,0,36,37,24,24,0,24,0,24,24,24,24,24,24,0,0,24,24,24,25,0,26,0,27,28,29,30,31,32,0,0,36,37,24,97,97,93,94,95,96,97,30,31,32,0,136,36,37,106,0,24,106,0,0,99,100,
 0,0,30,31,32,24,0,36,37,106,106,0,0,30,31,32,0,108,36,37,108,0,0,0,0,73,73,73,73,73,73,73,73,73,108,108,0,0,73,0,93,0,0,93,106,73,73,73,73,0,0,0,0,45,45,45,45,93,93,0,0,0,67,67,67,67,45,0,0,108,0,0,0,45,45,67,0,0,0,0,0,89,67,67,90,0,87,85,0,86,0,88,93,0,62,62,62,62,63,0,0,0,63,63,63,63,63,62,63,0,98,0,0,0,62,62,0,0,0,63,63,63,64,63,63,63,64,64,64,64,64,0,64,0,0,0,0,0,0,0,0,0,0,64,64,64,0,64,64,64,89,0,0,90,63,87,85,0,86,89,88,0,90,0,87,85,0,86,0,88,0,0,91,82,0,83,79,84,64,0,0,107,82,0,83,79,
 84,89,0,0,90,109,87,85,0,86,89,88,0,90,135,87,85,0,86,0,88,0,0,0,82,0,83,79,84,0,0,0,0,82,0,83,79,84,89,0,0,90,157,87,85,0,86,89,88,0,90,0,87,85,0,86,0,88,0,0,0,82,0,83,79,84,0,0,163,0,82,0,83,79,84,89,0,0,90,0,87,85,164,86,89,88,0,90,177,87,85,0,86,0,88,0,0,0,82,0,83,79,84,0,0,0,0,82,0,83,79,84,0,0,89,0,0,90,0,87,85,0,86,0,88,93,94,95,96,97,0,0,0,63,63,63,63,82,0,83,79,84,45,99,100,45,63,45,45,0,45,0,45,63,63,0,0,64,64,64,64,0,0,0,45,45,0,45,45,45,64,0,0,0,0,0,0,64,64,0,0,0,0,0,0,74,75,76,
 77,0,0,0,0,0,74,75,76,77,78,0,0,0,0,0,0,80,81,78,0,0,0,0,0,0,80,81,0,0,0,0,0,0,74,75,76,77,0,0,0,0,0,74,75,76,77,78,0,0,0,0,0,0,80,81,78,0,0,0,0,0,0,80,81,0,0,0,0,0,0,74,75,76,77,0,0,0,0,0,74,75,76,77,78,0,0,0,0,0,0,80,81,78,0,0,0,0,0,0,80,81,0,0,0,0,0,0,74,75,76,77,0,0,0,0,0,74,75,76,77,78,0,0,0,0,0,0,80,81,78,0,0,0,0,0,89,80,81,90,0,87,85,0,86,0,88,74,75,76,77,0,0,0,0,0,0,0,0,82,78,83,66,84,66,66,66,80,81,0,0,0,0,0,0,45,45,45,45,66,66,66,0,66,66,66,65,0,65,65,65,0,0,0,0,45,45,0,0,0,0,0,0,
 65,65,65,0,65,65,65,0,0,0,60,66,60,60,60,0,0,0,0,61,0,61,61,61,0,0,0,60,60,60,0,60,60,60,0,65,61,61,61,0,61,61,61,0,0,0,68,0,0,68,0,0,0,0,0,50,0,0,50,0,0,0,60,68,68,68,0,68,68,68,0,61,50,50,50,0,50,50,51,0,0,51,0,0,52,0,0,52,0,0,0,0,0,0,0,51,51,51,68,51,51,52,52,52,0,52,52,50,0,0,53,0,0,53,0,0,55,0,0,55,0,0,0,0,0,0,0,53,53,53,51,53,53,55,55,55,52,55,55,0,0,0,54,0,0,54,0,0,0,0,0,0,0,0,74,75,76,77,0,54,54,54,53,54,54,0,0,78,55,0,0,0,0,0,0,81,66,66,66,66,0,0,89,0,0,90,0,87,85,66,86,0,88,0,54,
 0,66,66,0,0,65,65,65,65,0,82,0,83,0,84,0,0,0,65,0,0,0,0,0,0,65,65,0,0,0,0,0,60,60,60,60,0,0,0,0,0,61,61,61,61,60,0,0,0,0,0,0,60,60,61,0,0,0,0,0,0,61,61,0,0,0,0,0,68,68,68,68,0,0,0,0,0,50,50,50,50,68,0,0,0,0,0,0,68,68,50,0,0,0,0,0,0,50,50,0,51,51,51,51,0,0,52,52,52,52,0,0,0,51,0,0,0,0,0,52,51,51,0,0,0,0,52,52,0,0,0,0,53,53,53,53,0,0,55,55,55,55,0,0,0,53,0,0,0,0,0,55,53,53,0,0,0,0,55,55,0,0,0,0,54,54,54,54,0,64,0,0,0,66,67,68,0,54,71,0,0,0,0,0,54,54,0,0,0,0,0,0,0,0,0,0,0,0,0,101,102,0,0,0,0,
 0,0,0,74,75,76,77,0,0,111,112,113,114,0,0,0,78,119,120,121,122,123,124,125,126,0,0,0,0,0,0,0,0,134,0,0,0,0,0,0,0,139,0,0,0,0,0,0,0,0,0,142,143,144,0,0,0,0,0,0,0,0,145,0,148,149,150,151,152,0,0,0,0,0,0,0,161,0,0,0,0,0,0,0,0,0,0,0,0,0,102,169,0,0,0,0,0,0,0,145,0,0,0,0,0,0,0,0,180,0,0,0,0,0,183,0,0,102,};
static const fawk_int_t fawk_check[] = {33,41,46,46,44,38,41,40,257,155,43,41,45,44,44,33,275,44,58,59,38,40,40,63,59,43,59,45,58,59,33,41,44,46,44,38,182,40,275,276,43,59,45,41,41,33,44,44,58,59,38,41,40,93,44,43,59,45,41,46,33,44,93,93,
46,38,93,40,58,59,43,59,45,36,37,58,59,33,91,42,63,93,38,93,40,123,59,43,33,45,123,46,125,38,61,40,61,60,43,41,45,37,44,93,40,123,42,125,46,40,93,47,40,275,269,78,58,59,40,40,123,37,125,273,40,41,42,43,44,45,59,47,40,59,41,123,41,93,41,0,59,41,58,59,60,61,62,63,64,37,41,93,40,41,42,43,44,45,37,47,41,128,41,42,43,44,45,5,47,39,58,59,60,136,62,63,64,93,164,58,59,60,-1,62,63,64,153,-1,-1,-1,37,-1,-1,-1,41,42,43,44,45,-1,47,-1,-1,170,-1,93,-1,-1,175,-1,-1,58,59,60,93,62,63,64,37,186,-1,40,-1,42,
 43,-1,45,-1,47,-1,-1,275,275,-1,267,268,-1,270,278,272,273,274,275,276,277,64,93,280,281,267,268,-1,270,-1,272,273,274,275,276,277,-1,-1,280,281,267,268,-1,270,-1,272,273,274,275,276,277,-1,-1,280,281,267,268,-1,270,-1,272,273,274,275,276,277,-1,-1,280,281,267,278,279,262,263,264,265,266,275,276,277,-1,271,280,281,41,-1,267,44,-1,-1,280,281,-1,-1,275,276,277,267,-1,280,281,58,59,-1,-1,275,276,277,-1,41,280,281,44,-1,-1,-1,-1,258,259,260,261,262,263,264,265,266,58,59,-1,-1,271,-1,41,-1,-1,44,
 93,278,279,280,281,-1,-1,-1,-1,258,259,260,261,58,59,-1,-1,-1,258,259,260,261,271,-1,-1,93,-1,-1,-1,278,279,271,-1,-1,-1,-1,-1,37,278,279,40,-1,42,43,-1,45,-1,47,93,-1,258,259,260,261,37,-1,-1,-1,41,42,43,44,45,271,47,-1,61,-1,-1,-1,278,279,-1,-1,-1,58,59,60,37,62,63,64,41,42,43,44,45,-1,47,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,58,59,60,-1,62,63,64,37,-1,-1,40,93,42,43,-1,45,37,47,-1,40,-1,42,43,-1,45,-1,47,-1,-1,59,60,-1,62,63,64,93,-1,-1,59,60,-1,62,63,64,37,-1,-1,40,41,42,43,-1,45,37,47,-1,40,41,
 42,43,-1,45,-1,47,-1,-1,-1,60,-1,62,63,64,-1,-1,-1,-1,60,-1,62,63,64,37,-1,-1,40,41,42,43,-1,45,37,47,-1,40,-1,42,43,-1,45,-1,47,-1,-1,-1,60,-1,62,63,64,-1,-1,58,-1,60,-1,62,63,64,37,-1,-1,40,-1,42,43,44,45,37,47,-1,40,41,42,43,-1,45,-1,47,-1,-1,-1,60,-1,62,63,64,-1,-1,-1,-1,60,-1,62,63,64,-1,-1,37,-1,-1,40,-1,42,43,-1,45,-1,47,262,263,264,265,266,-1,-1,-1,258,259,260,261,60,-1,62,63,64,37,280,281,40,271,42,43,-1,45,-1,47,278,279,-1,-1,258,259,260,261,-1,-1,-1,59,60,-1,62,63,64,271,-1,-1,-1,
 -1,-1,-1,278,279,-1,-1,-1,-1,-1,-1,258,259,260,261,-1,-1,-1,-1,-1,258,259,260,261,271,-1,-1,-1,-1,-1,-1,278,279,271,-1,-1,-1,-1,-1,-1,278,279,-1,-1,-1,-1,-1,-1,258,259,260,261,-1,-1,-1,-1,-1,258,259,260,261,271,-1,-1,-1,-1,-1,-1,278,279,271,-1,-1,-1,-1,-1,-1,278,279,-1,-1,-1,-1,-1,-1,258,259,260,261,-1,-1,-1,-1,-1,258,259,260,261,271,-1,-1,-1,-1,-1,-1,278,279,271,-1,-1,-1,-1,-1,-1,278,279,-1,-1,-1,-1,-1,-1,258,259,260,261,-1,-1,-1,-1,-1,258,259,260,261,271,-1,-1,-1,-1,-1,-1,278,279,271,-1,
 -1,-1,-1,-1,37,278,279,40,-1,42,43,-1,45,-1,47,258,259,260,261,-1,-1,-1,-1,-1,-1,-1,-1,60,271,62,41,64,43,44,45,278,279,-1,-1,-1,-1,-1,-1,258,259,260,261,58,59,60,-1,62,63,64,41,-1,43,44,45,-1,-1,-1,-1,278,279,-1,-1,-1,-1,-1,-1,58,59,60,-1,62,63,64,-1,-1,-1,41,93,43,44,45,-1,-1,-1,-1,41,-1,43,44,45,-1,-1,-1,58,59,60,-1,62,63,64,-1,93,58,59,60,-1,62,63,64,-1,-1,-1,41,-1,-1,44,-1,-1,-1,-1,-1,41,-1,-1,44,-1,-1,-1,93,58,59,60,-1,62,63,64,-1,93,58,59,60,-1,62,63,41,-1,-1,44,-1,-1,41,-1,-1,44,-1,
 -1,-1,-1,-1,-1,-1,58,59,60,93,62,63,58,59,60,-1,62,63,93,-1,-1,41,-1,-1,44,-1,-1,41,-1,-1,44,-1,-1,-1,-1,-1,-1,-1,58,59,60,93,62,63,58,59,60,93,62,63,-1,-1,-1,41,-1,-1,44,-1,-1,-1,-1,-1,-1,-1,-1,258,259,260,261,-1,58,59,60,93,62,63,-1,-1,271,93,-1,-1,-1,-1,-1,-1,279,258,259,260,261,-1,-1,37,-1,-1,40,-1,42,43,271,45,-1,47,-1,93,-1,278,279,-1,-1,258,259,260,261,-1,60,-1,62,-1,64,-1,-1,-1,271,-1,-1,-1,-1,-1,-1,278,279,-1,-1,-1,-1,-1,258,259,260,261,-1,-1,-1,-1,-1,258,259,260,261,271,-1,-1,-1,
 -1,-1,-1,278,279,271,-1,-1,-1,-1,-1,-1,278,279,-1,-1,-1,-1,-1,258,259,260,261,-1,-1,-1,-1,-1,258,259,260,261,271,-1,-1,-1,-1,-1,-1,278,279,271,-1,-1,-1,-1,-1,-1,278,279,-1,258,259,260,261,-1,-1,258,259,260,261,-1,-1,-1,271,-1,-1,-1,-1,-1,271,278,279,-1,-1,-1,-1,278,279,-1,-1,-1,-1,258,259,260,261,-1,-1,258,259,260,261,-1,-1,-1,271,-1,-1,-1,-1,-1,271,278,279,-1,-1,-1,-1,278,279,-1,-1,-1,-1,258,259,260,261,-1,29,-1,-1,-1,33,34,35,-1,271,38,-1,-1,-1,-1,-1,278,279,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
 -1,-1,-1,59,60,-1,-1,-1,-1,-1,-1,-1,258,259,260,261,-1,-1,74,75,76,77,-1,-1,-1,271,82,83,84,85,86,87,88,89,-1,-1,-1,-1,-1,-1,-1,-1,98,-1,-1,-1,-1,-1,-1,-1,106,-1,-1,-1,-1,-1,-1,-1,-1,-1,116,117,118,-1,-1,-1,-1,-1,-1,-1,-1,127,-1,129,130,131,132,133,-1,-1,-1,-1,-1,-1,-1,141,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,155,156,-1,-1,-1,-1,-1,-1,-1,164,-1,-1,-1,-1,-1,-1,-1,-1,173,-1,-1,-1,-1,-1,179,-1,-1,182,};
#define fawk_FINAL 1
#define fawk_MAXTOKEN 281
#define fawk_UNDFTOKEN 332
#define fawk_TRANSLATE(a) ((a) > fawk_MAXTOKEN ? fawk_UNDFTOKEN : (a))
FAWK_API void fawk_error(fawk_ctx_t *ctx, fawk_STYPE tok, const char *msg) { libfawk_error(ctx, msg, ctx->parser.isp->fn, ctx->parser.isp->line+1, ctx->parser.isp->col+1); }
FAWK_API int fawk_lex_fawk(fawk_STYPE *lval, fawk_ctx_t *ctx);
FAWK_API int fawk_parse_fawk(fawk_ctx_t *ctx) { fawk_parser_loop(fawk_yyctx_t, fawk_STYPE, fawk_lex_fawk, fawk_parse, ctx, fawk_RES_NEXT, fawk_RES_DONE); }
static int fawk_growstack(fawk_yyctx_t *yyctx, fawk_STACKDATA *data)
{
 int i;
 unsigned newsize;
 fawk_int_t *newss;
 fawk_STYPE *newvs;
 if ((newsize = data->stacksize) == 0)
  newsize = fawk_INITSTACKSIZE;
 else if (newsize >= yyctx->stack_max_depth)
  return fawk_ENOMEM;
 else if ((newsize *= 2) > yyctx->stack_max_depth)
  newsize = yyctx->stack_max_depth;
 i = (int)(data->s_mark - data->s_base);
 newss = (fawk_int_t *) realloc(data->s_base, newsize * sizeof(*newss));
 if (newss == 0)
  return fawk_ENOMEM;
 data->s_base = newss;
 data->s_mark = newss + i;
 newvs = (fawk_STYPE *) realloc(data->l_base, newsize * sizeof(*newvs));
 if (newvs == 0)
  return fawk_ENOMEM;
 data->l_base = newvs;
 data->l_mark = newvs + i;
 data->stacksize = newsize;
 data->s_last = data->s_base + newsize - 1;
 return 0;
}
static void fawk_freestack(fawk_STACKDATA *data)
{
 free(data->s_base);
 free(data->l_base);
 memset(data, 0, sizeof(*data));
}
#define fawk_ABORT goto yyabort
#define fawk_REJECT goto yyabort
#define fawk_ACCEPT goto yyaccept
#define fawk_ERROR goto yyerrlab
int fawk_parse_init(fawk_yyctx_t *yyctx)
{
 memset(&yyctx->val, 0, sizeof(yyctx->val));
 memset(&yyctx->lval, 0, sizeof(yyctx->lval));
 yyctx->yym = 0;
 yyctx->yyn = 0;
 yyctx->nerrs = 0;
 yyctx->errflag = 0;
 yyctx->chr = fawk_EMPTY;
 yyctx->state = 0;
 memset(&yyctx->stack, 0, sizeof(yyctx->stack));
 yyctx->stack_max_depth = fawk_INITSTACKSIZE > 10000 ? fawk_INITSTACKSIZE : 10000;
 if (yyctx->stack.s_base == NULL && fawk_growstack(yyctx, &yyctx->stack) == fawk_ENOMEM)
  return -1;
 yyctx->stack.s_mark = yyctx->stack.s_base;
 yyctx->stack.l_mark = yyctx->stack.l_base;
 yyctx->state = 0;
 *yyctx->stack.s_mark = 0;
 yyctx->jump = 0;
 return 0;
}
#define fawk_GETCHAR(labidx) \
do { \
 if (used) { yyctx->jump = labidx; return fawk_RES_NEXT; } \
 getchar_ ## labidx:; yyctx->chr = tok; yyctx->lval = *lval; used = 1; \
} while(0)
fawk_res_t fawk_parse(fawk_yyctx_t *yyctx, fawk_ctx_t *ctx, int tok, fawk_STYPE *lval)
{
 int used = 0;
yyloop:;
 if (yyctx->jump == 1) { yyctx->jump = 0; goto getchar_1; }
 if (yyctx->jump == 2) { yyctx->jump = 0; goto getchar_2; }
 if ((yyctx->yyn = fawk_defred[yyctx->state]) != 0)
  goto yyreduce;
 if (yyctx->chr < 0) {
  fawk_GETCHAR(1);
  if (yyctx->chr < 0)
   yyctx->chr = fawk_EOF;
 }
 if (((yyctx->yyn = fawk_sindex[yyctx->state]) != 0) && (yyctx->yyn += yyctx->chr) >= 0 && yyctx->yyn <= fawk_TABLESIZE && fawk_check[yyctx->yyn] == (fawk_int_t) yyctx->chr) {
  if (yyctx->stack.s_mark >= yyctx->stack.s_last && fawk_growstack(yyctx, &yyctx->stack) == fawk_ENOMEM)
   goto yyoverflow;
  yyctx->state = fawk_table[yyctx->yyn];
  *++yyctx->stack.s_mark = fawk_table[yyctx->yyn];
  *++yyctx->stack.l_mark = yyctx->lval;
  yyctx->chr = fawk_EMPTY;
  if (yyctx->errflag > 0)
   --yyctx->errflag;
  goto yyloop;
 }
 if (((yyctx->yyn = fawk_rindex[yyctx->state]) != 0) && (yyctx->yyn += yyctx->chr) >= 0 && yyctx->yyn <= fawk_TABLESIZE && fawk_check[yyctx->yyn] == (fawk_int_t) yyctx->chr) {
  yyctx->yyn = fawk_table[yyctx->yyn];
  goto yyreduce;
 }
 if (yyctx->errflag != 0)
  goto yyinrecovery;
 fawk_error(ctx, yyctx->lval, "syntax error");
 goto yyerrlab;
yyerrlab:
 ++yyctx->nerrs;
yyinrecovery:
 if (yyctx->errflag < 3) {
  yyctx->errflag = 3;
  for(;;) {
   if (((yyctx->yyn = fawk_sindex[*yyctx->stack.s_mark]) != 0) && (yyctx->yyn += fawk_ERRCODE) >= 0 && yyctx->yyn <= fawk_TABLESIZE && fawk_check[yyctx->yyn] == (fawk_int_t) fawk_ERRCODE) {
    if (yyctx->stack.s_mark >= yyctx->stack.s_last && fawk_growstack(yyctx, &yyctx->stack) == fawk_ENOMEM)
     goto yyoverflow;
    yyctx->state = fawk_table[yyctx->yyn];
    *++yyctx->stack.s_mark = fawk_table[yyctx->yyn];
    *++yyctx->stack.l_mark = yyctx->lval;
    goto yyloop;
   }
   else {
    if (yyctx->stack.s_mark <= yyctx->stack.s_base)
     goto yyabort;
    --yyctx->stack.s_mark;
    --yyctx->stack.l_mark;
   }
  }
 }
 else {
  if (yyctx->chr == fawk_EOF)
   goto yyabort;
  yyctx->chr = fawk_EMPTY;
  goto yyloop;
 }
yyreduce:
 yyctx->yym = fawk_len[yyctx->yyn];
 if (yyctx->yym > 0)
  yyctx->val = yyctx->stack.l_mark[1 - yyctx->yym];
 else
  memset(&yyctx->val, 0, sizeof yyctx->val);
 switch (yyctx->yyn) {
case 1:  { fawkc_addi(ctx, FAWKI_ABORT); fawkc_addcs(ctx, "uninitialized execute"); }
break;
case 2:  { fawkc_addi(ctx, FAWKI_ABORT); fawkc_addcs(ctx, "ran beyond the script"); }
break;
case 5:  { ctx->compiler.numfixedargs = -1; ctx->compiler.numargs = 0; }
break;
case 6:  { fawk_symtab_regfunc(ctx, yyctx->stack.l_mark[-4].str, FAWK_CURR_IP(), ctx->compiler.numargs, ctx->compiler.numfixedargs); fawk_free(ctx, yyctx->stack.l_mark[-4].str); ctx->fp = ctx->sp; }
break;
case 7:  {
   int n;
   fawkc_addi(ctx, FAWKI_PUSH_NIL);
   fawkc_addi(ctx, FAWKI_RET);
   fawkc_addnum(ctx, ctx->compiler.numargs + (ctx->compiler.numfixedargs >= 0));
   for(n = 0; n < ctx->compiler.numargs + (ctx->compiler.numfixedargs >= 0); n++) {
    fawk_cell_t cell;
    fawk_pop(ctx, &cell);
    fawk_cell_free(ctx, &cell);
   }
   ctx->fp = 0;
  }
break;
case 9:  { fawk_push_str(ctx, yyctx->stack.l_mark[0].str); fawk_free(ctx, yyctx->stack.l_mark[0].str); ctx->compiler.numargs++; }
break;
case 10:  { ctx->compiler.numfixedargs = ctx->compiler.numargs; fawk_push_str(ctx, "VARARG"); }
break;
case 11:  { fawk_push_str(ctx, yyctx->stack.l_mark[0].str); fawk_free(ctx, yyctx->stack.l_mark[0].str); ctx->compiler.numargs++; }
break;
case 12:  { ctx->compiler.numfixedargs = ctx->compiler.numargs; fawk_push_str(ctx, "VARARG"); }
break;
case 13:  { fawkc_addi(ctx, FAWKI_POP); }
break;
case 18:  { fawkc_addi(ctx, FAWKI_RET); fawkc_addnum(ctx, ctx->compiler.numargs); }
break;
case 19:  { fawkc_addi(ctx, FAWKI_PUSH_NIL); fawkc_addi(ctx, FAWKI_RET); fawkc_addnum(ctx, ctx->compiler.numargs); }
break;
case 24:  {
    size_t jmp1 = fawk_pop_num(ctx, 1);
    ctx->code.code[jmp1].data.num = FAWK_CURR_IP();
   }
break;
case 25:  {
   fawkc_addi(ctx, FAWKI_JMP);
   FAWK_PUSH_IP(); fawkc_addnum(ctx, 777);
  }
break;
case 26:  {
   size_t jmp_then_post = fawk_pop_num(ctx, 1), jmp_if = fawk_pop_num(ctx, 1);
   ctx->code.code[jmp_then_post].data.num = FAWK_CURR_IP();
   ctx->code.code[jmp_if].data.num = jmp_then_post+1;
  }
break;
case 27:  { fawkc_addi(ctx, FAWKI_POPJZ); FAWK_PUSH_IP(); fawkc_addnum(ctx, 777); }
break;
case 31:  {
    fawkc_addi(ctx, FAWKI_FORIN_FIRST);
    fawkc_addi(ctx, FAWKI_JMP);
    fawkc_addnum(ctx, 0);
    FAWK_PUSH_IP();
   }
break;
case 32:  {
    size_t begin = fawk_pop_num(ctx, 1);
    fawkc_addi(ctx, FAWKI_FORIN_NEXT);
    fawkc_addnum(ctx, begin);
    ctx->code.code[begin-1].data.num = FAWK_CURR_IP()-2;
   }
break;
case 33:  {
    fawkc_addi(ctx, FAWKI_POP);
    FAWK_PUSH_IP();
   }
break;
case 34:  {
    fawkc_addi(ctx, FAWKI_POPJZ);
    FAWK_PUSH_IP();
    fawkc_addnum(ctx, 0);
    fawkc_addi(ctx, FAWKI_JMP);
    FAWK_PUSH_IP();
    fawkc_addnum(ctx, 0);
   }
break;
case 35:  {
    fawkc_addi(ctx, FAWKI_POP);
    fawkc_addi(ctx, FAWKI_JMP);
    FAWK_PUSH_IP();
    fawkc_addnum(ctx, 0);
   }
break;
case 36:  {
    size_t jback3rd, jskip, jout, rerun;
    jback3rd = fawk_pop_num(ctx, 1);
    jskip = fawk_pop_num(ctx, 1);
    jout = fawk_pop_num(ctx, 1);
    rerun = fawk_pop_num(ctx, 1);
    fawkc_addi(ctx, FAWKI_JMP);
    fawkc_addnum(ctx, jskip+1);
    ctx->code.code[jback3rd].data.num = rerun;
    ctx->code.code[jskip].data.num = jback3rd+1;
    ctx->code.code[jout].data.num = FAWK_CURR_IP();
   }
break;
case 37:  { FAWK_PUSH_IP(); }
break;
case 38:  { fawkc_addi(ctx, FAWKI_POPJNZ); fawkc_addnum(ctx, fawk_pop_num(ctx, 1)); }
break;
case 39:  { FAWK_PUSH_IP(); }
break;
case 40:  { fawkc_addi(ctx, FAWKI_POPJZ); FAWK_PUSH_IP(); fawkc_addnum(ctx, 0); }
break;
case 41:  { loop_pretest_jumpback(); }
break;
case 42:  { fawkc_addi(ctx, FAWKI_PUSH_NUM); fawkc_addnum(ctx, yyctx->stack.l_mark[0].num); }
break;
case 43:  { fawkc_addi(ctx, FAWKI_PUSH_STR); fawkc_adds(ctx, yyctx->stack.l_mark[0].str); fawk_free(ctx, yyctx->stack.l_mark[0].str); }
break;
case 44:  { fawkc_addi(ctx, FAWKI_PUSH_NIL); }
break;
case 45:  { fawkc_addi(ctx, FAWKI_PUSH_SYMVAL); }
break;
case 50:  { fawkc_addi(ctx, FAWKI_EQ); }
break;
case 51:  { fawkc_addi(ctx, FAWKI_NEQ); }
break;
case 52:  { fawkc_addi(ctx, FAWKI_GTEQ); }
break;
case 53:  { fawkc_addi(ctx, FAWKI_LTEQ); }
break;
case 54:  { fawkc_addi(ctx, FAWKI_GT); }
break;
case 55:  { fawkc_addi(ctx, FAWKI_LT); }
break;
case 56:  { fawkc_addi(ctx, FAWKI_IN); }
break;
case 60:  { fawkc_addi(ctx, FAWKI_ADD); }
break;
case 61:  { fawkc_addi(ctx, FAWKI_SUB); }
break;
case 62:  { fawkc_addi(ctx, FAWKI_MUL); }
break;
case 63:  { fawkc_addi(ctx, FAWKI_DIV); }
break;
case 64:  { fawkc_addi(ctx, FAWKI_MOD); }
break;
case 65:  { fawkc_addi(ctx, FAWKI_NEG); }
break;
case 67:  { fawkc_addi(ctx, FAWKI_NOT); }
break;
case 68:  { fawkc_addi(ctx, FAWKI_CONCAT); }
break;
case 71:  { fawkc_addi(ctx, FAWKI_PUSH_NUM); fawkc_addnum(ctx, 1); }
break;
case 72:  { fawk_push_num(ctx, ctx->compiler.numidx); ctx->compiler.numidx = 0; }
break;
case 73:  {
   fawkc_addi(ctx, FAWKI_MAKE_SYMREF);
   fawkc_addsymref(ctx, yyctx->stack.l_mark[-2].str, ctx->compiler.numidx, 0);
   fawkc_addnum(ctx, ctx->compiler.numidx);
   fawk_free(ctx, yyctx->stack.l_mark[-2].str);
   ctx->compiler.numidx = fawk_pop_num(ctx, 1);
  }
break;
case 74:  { fawkc_addi(ctx, FAWKI_INCDEC); fawkc_addnum(ctx, FAWK_INCDEC_INC | FAWK_INCDEC_POST); }
break;
case 75:  { fawkc_addi(ctx, FAWKI_INCDEC); fawkc_addnum(ctx, FAWK_INCDEC_POST); }
break;
case 76:  { fawkc_addi(ctx, FAWKI_INCDEC); fawkc_addnum(ctx, FAWK_INCDEC_INC); }
break;
case 77:  { fawkc_addi(ctx, FAWKI_INCDEC); fawkc_addnum(ctx, 0); }
break;
case 79:  { ctx->compiler.numidx = -1; }
break;
case 80:  { ctx->compiler.numidx++; }
break;
case 81:  { ctx->compiler.numidx++; fawkc_addi(ctx, FAWKI_PUSH_STR); fawkc_adds(ctx, yyctx->stack.l_mark[0].str); fawk_free(ctx, yyctx->stack.l_mark[0].str); }
break;
case 82:  { ctx->compiler.numidx++; fawkc_addi(ctx, FAWKI_PUSH_STR); fawkc_adds(ctx, yyctx->stack.l_mark[0].str); fawk_free(ctx, yyctx->stack.l_mark[0].str); }
break;
case 84:  { parse_aix_expr(); }
break;
case 85:  { fawkc_addi(ctx, FAWKI_CONCAT); }
break;
case 86:  { fawk_push_num(ctx, ctx->compiler.numargs); ctx->compiler.numargs = 0; ctx->code.used--; }
break;
case 87:  {
   size_t old_numargs = fawk_pop_num(ctx, 1);
   fawkc_addi(ctx, FAWKI_CALL);
   fawkc_addnum(ctx, ctx->compiler.numargs);
   ctx->compiler.numargs = old_numargs;
  }
break;
case 89:  { ctx->compiler.numargs++; }
break;
case 90:  { ctx->compiler.numargs++; }
break;
case 91:  { fawkc_addi(ctx, FAWKI_POPJZ); FAWK_PUSH_IP(); fawkc_addnum(ctx, 777); }
break;
case 92:  { fawkc_addi(ctx, FAWKI_JMP); FAWK_PUSH_IP(); fawkc_addnum(ctx, 888); }
break;
case 93:  {
   size_t jmp1, jmp2;
   jmp2 = fawk_pop_num(ctx, 1); jmp1 = fawk_pop_num(ctx, 1);
   ctx->code.code[jmp1].data.num = jmp2+1;
   ctx->code.code[jmp2].data.num = FAWK_CURR_IP();
  }
break;
case 94:  { lazy_binop1(ctx, 1); }
break;
case 95:  { lazy_binop2(ctx, 1); }
break;
case 96:  { lazy_binop1(ctx, 0); }
break;
case 97:  { lazy_binop2(ctx, 0); }
break;
case 98:  { fawkc_addi(ctx, FAWKI_SET); fawkc_addi(ctx, FAWKI_PUSH_SYMVAL); }
break;
case 99:  { fawkc_addi(ctx, FAWKI_PUSH_TOPVAR); }
break;
case 100:  { fawkc_addi(ctx, FAWKI_ADD); fawkc_addi(ctx, FAWKI_SET); }
break;
case 101:  { fawkc_addi(ctx, FAWKI_PUSH_TOPVAR); }
break;
case 102:  { fawkc_addi(ctx, FAWKI_SUB); fawkc_addi(ctx, FAWKI_SET); }
break;
case 103:  { fawkc_addi(ctx, FAWKI_PUSH_TOPVAR); }
break;
case 104:  { fawkc_addi(ctx, FAWKI_MUL); fawkc_addi(ctx, FAWKI_SET); }
break;
case 105:  { fawkc_addi(ctx, FAWKI_PUSH_TOPVAR); }
break;
case 106:  { fawkc_addi(ctx, FAWKI_DIV); fawkc_addi(ctx, FAWKI_SET); }
break;
case 107:  { fawkc_addi(ctx, FAWKI_PUSH_TOPVAR); }
break;
case 108:  { fawkc_addi(ctx, FAWKI_MOD); fawkc_addi(ctx, FAWKI_SET); }
break;
 }
 yyctx->stack.s_mark -= yyctx->yym;
 yyctx->state = *yyctx->stack.s_mark;
 yyctx->stack.l_mark -= yyctx->yym;
 yyctx->yym = fawk_lhs[yyctx->yyn];
 if (yyctx->state == 0 && yyctx->yym == 0) {
  yyctx->state = fawk_FINAL;
  *++yyctx->stack.s_mark = fawk_FINAL;
  *++yyctx->stack.l_mark = yyctx->val;
  if (yyctx->chr < 0) {
   fawk_GETCHAR(2);
   if (yyctx->chr < 0)
    yyctx->chr = fawk_EOF;
  }
  if (yyctx->chr == fawk_EOF)
   goto yyaccept;
  goto yyloop;
 }
 if (((yyctx->yyn = fawk_gindex[yyctx->yym]) != 0) && (yyctx->yyn += yyctx->state) >= 0 && yyctx->yyn <= fawk_TABLESIZE && fawk_check[yyctx->yyn] == (fawk_int_t) yyctx->state)
  yyctx->state = fawk_table[yyctx->yyn];
 else
  yyctx->state = fawk_dgoto[yyctx->yym];
 if (yyctx->stack.s_mark >= yyctx->stack.s_last && fawk_growstack(yyctx, &yyctx->stack) == fawk_ENOMEM)
  goto yyoverflow;
 *++yyctx->stack.s_mark = (fawk_int_t) yyctx->state;
 *++yyctx->stack.l_mark = yyctx->val;
 goto yyloop;
yyoverflow:
 fawk_error(ctx, yyctx->lval, "yacc stack overflow");
yyabort:
 fawk_freestack(&yyctx->stack);
 return fawk_RES_ABORT;
yyaccept:
 fawk_freestack(&yyctx->stack);
 return fawk_RES_DONE;
}
FAWK_API int fawk_lex_fawk(fawk_STYPE *lval, fawk_ctx_t *ctx)
{
 int chr, nchr;
 char tmp[128];
 restart:;
 if (ctx->parser.in_eof) goto handle_eof;
 lex_textblk_exec(T_STRING, goto restart, '\n');
 ctx->parser.used = 0;
 if (isalpha(chr) || (chr == '_')) {
  append(chr, return -1);
  for(;;) {
   chr = getch(ctx);
   if (!(isalpha(chr)) && !(isdigit(chr)) && (chr != '_')) {
    ungetch(ctx, chr);
    break;
   }
   append(chr, return -1);
  }
  append('\0', return -1);
  if (strcmp(ctx->parser.buff, "function") == 0) return T_FUNCTION;
  else if (strcmp(ctx->parser.buff, "for") == 0) return T_FOR;
  else if (strcmp(ctx->parser.buff, "do") == 0) return T_DO;
  else if (strcmp(ctx->parser.buff, "while") == 0) return T_WHILE;
  else if (strcmp(ctx->parser.buff, "if") == 0) return T_IF;
  else if (strcmp(ctx->parser.buff, "else") == 0) return T_ELSE;
  else if (strcmp(ctx->parser.buff, "in") == 0) return T_IN;
  else if (strcmp(ctx->parser.buff, "return") == 0) return T_RETURN;
  else if (strcmp(ctx->parser.buff, "include") == 0) { lex_include(goto restart); }
  else {
   lval->str = fawk_strdup(ctx, ctx->parser.buff);
   return (lval->str == NULL) ? -1 : T_ID;
  }
 }
 else if (isdigit(chr)) {
  append(chr, return -1);
  if (chr == '0') {
   nchr = getch(ctx);
   if (nchr == 'x') {
    fawk_readup(ctx, "0123456789abcdefABCDEF");
    lval->num = strtol(ctx->parser.buff, NULL, 16);
    return T_NUM;
   }
   ungetch(ctx, nchr);
  }
  return read_numeric(ctx, &lval->num, 0, T_NUM);
 }
 else switch(chr) {
  case '@': case '?': case ':': case '(': case ')':
  case '\\': case ']': case ',': case '{': case '}':
   return chr;
  case '[': { lex_textblk_start(goto restart); }
  case '.':
   nchr = getch(ctx);
   if (!isdigit(nchr)) {
    ungetch(ctx, nchr);
    return '.';
   }
   else {
    append('.', return -1); append(nchr, return -1);
    return read_numeric(ctx, &lval->num, 1, T_NUM);
   }
   break;
  case '+':
   nchr = getch(ctx); append(chr, return -1);
   switch(nchr) {
    case '+': return T_PP;
    case '=': return T_PLEQ;
    default: ungetch(ctx, nchr); return chr;
   }
   break;
  case '-':
   nchr = getch(ctx); append(chr, return -1);
   switch(nchr) {
    case '-': return T_MM;
    case '=': return T_MIEQ;
    default: ungetch(ctx, nchr); return chr;
   }
   break;
  case_op_ch2('/', '=', T_DIEQ);
  case_op_ch2('*', '=', T_MUEQ);
  case_op_ch2('%', '=', T_MOEQ);
  case_op_ch2('=', '=', T_EQ);
  case_op_ch2('!', '=', T_NEQ);
  case_op_ch2('<', '=', T_LTEQ);
  case_op_ch2('>', '=', T_GTEQ);
  case_op_ch2('&', '&', T_AND);
  case_op_ch2('|', '|', T_OR);
  case ';':
   return ';';
  case '#':
   readtil(ctx, "\n\r");
   fawk_readup(ctx, " \t\r\n");
   goto restart;
  case '\"':
   if (read_strlit(ctx, '\"') == 0) return T_NIL;
   lval->str = fawk_strdup(ctx, ctx->parser.buff);
   return (lval->str == NULL) ? -1 : T_STRING;
  case EOF: { lex_got_eof(goto restart); }
  default:
   sprintf(tmp, "Invalid character on input: '%c'\n", chr);
   LIBFAWK_ERROR(ctx, tmp, ctx->parser.isp->fn, ctx->parser.isp->line+1, ctx->parser.isp->col+1, -1);
 }
}
