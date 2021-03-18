/* open addressing hash table */
/* max size is 1 << 31 */
/* an entry pointer is valid until the next insertion or resize */
/*
typedef void *HT(key_t);
typedef void *HT(value_t);

Plus optionally, for key const correctness:

typedef void *HT(const key_t);
#define HT_HAS_CONST_KEY

*/

#ifndef HT_HAS_CONST_KEY
	typedef HT(key_t) HT(const_key_t);
#else
#	undef HT_HAS_CONST_KEY
#endif

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
#ifdef GENHT_USER_FIELDS
	GENHT_USER_FIELDS
#endif
} HT(t);

/* allocates a new hash table, but the function may return null-pointer */
HT(t) *HT(alloc)(unsigned int (*keyhash)(HT(const_key_t)), int (*keyeq)(HT(const_key_t), HT(const_key_t)));
/* returns 0 on success */
int HT(init)(HT(t) *ht, unsigned int (*keyhash)(HT(const_key_t)), int (*keyeq)(HT(const_key_t), HT(const_key_t)));
void HT(free)(HT(t) *ht);
void HT(uninit)(HT(t) *ht);
void HT(clear)(HT(t) *ht);
HT(t) *HT(copy)(const HT(t) *ht);
/* new size is 2^n >= hint, returns 0 on success */
int HT(resize)(HT(t) *ht, unsigned int hint);

/* ht[key] is used */
int HT(has)(HT(t) *ht, HT(const_key_t) key);
/* value of ht[key] or 0 if key is not used */
HT(value_t) HT(get)(HT(t) *ht, HT(const_key_t) key);
/* entry of ht[key] or NULL if key is not used */
HT(entry_t) *HT(getentry)(HT(t) *ht, HT(const_key_t) key);
/* ht[key] = value */
void HT(set)(HT(t) *ht, HT(key_t) key, HT(value_t) value);
/* if key is used then return ht[key] else ht[key] = value and return NULL */
/* (the value of the returned used entry can be modified) */
HT(entry_t) *HT(insert)(HT(t) *ht, HT(key_t) key, HT(value_t) value);
/* delete key and return ht[key] or 0 if key is not used */
HT(value_t) HT(pop)(HT(t) *ht, HT(const_key_t) key);
/* delete key and return ht[key] or NULL if key is not used */
/* (the returned deleted entry can be used to free key,value resources) */
HT(entry_t) *HT(popentry)(HT(t) *ht, HT(const_key_t) key);
/* delete entry (useful for destructive iteration) */
void HT(delentry)(HT(t) *ht, HT(entry_t) *entry);


/* User application can override malloc/realloc/free by defining these macros: */
#ifndef genht_malloc
#define genht_malloc(ht, size) malloc(size)
#endif
#ifndef genht_calloc
#define genht_calloc(ht, size1, size2) calloc(size1, size2)
#endif
#ifndef genht_realloc
#define genht_realloc(ht, ptr, size) realloc(ptr, size)
#endif
#ifndef genht_free
#define genht_free(ht, ptr) free(ptr)
#endif


#ifdef GENHT_WANT_INLINE
#	define GENHT_STATIC static
#	define GENHT_INLINE inline
#	include "ht_inlines.h"
#else
/* helper functions */
unsigned int HT(length)(const HT(t) *ht);
unsigned int HT(fill)(const HT(t) *ht);
unsigned int HT(size)(const HT(t) *ht);

/* for any entry exactly one returns true */
int HT(isused)(const HT(entry_t) *entry);
int HT(isempty)(const HT(entry_t) *entry);
int HT(isdeleted)(const HT(entry_t) *entry);

/* first used (useful for iteration) */
HT(entry_t) *HT(first)(const HT(t) *ht);

/* next used (useful for iteration) */
HT(entry_t) *HT(next)(const HT(t) *ht, HT(entry_t) *entry);

#endif
