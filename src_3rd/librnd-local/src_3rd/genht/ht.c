#include <stdlib.h>

#ifdef GENHT_WANT_INLINE
#	define GENHT_INLINE inline
#	define GENHT_STATIC static
#else
/* make sure inline and static are empty so all calls become linkable functions */
#	define GENHT_INLINE
#	define GENHT_STATIC
#	include "ht_inlines.h"
#endif


#ifndef HT_INVALID_VALUE
#define HT_INVALID_VALUE 0
#endif

#define HT_MINSIZE 8
#define HT_MAXSIZE (1U << 31)

#define JUMP(i, j) i += j++
#define JUMP_FIRST(i, j) j = 1, i += j++

/* generic functions, useful if ht_entry_t changes */

static GENHT_INLINE void setused(HT(entry_t) *entry) {
	entry->flag = 1;
}

static GENHT_INLINE void setdeleted(HT(entry_t) *entry) {
	entry->flag = -1;
}

static GENHT_INLINE unsigned int entryhash(const HT(entry_t) *entry) {
	return entry->hash;
}

int HT(init)(HT(t) *ht, unsigned int (*keyhash)(HT(const_key_t)), int (*keyeq)(HT(const_key_t), HT(const_key_t))) {
	ht->mask = HT_MINSIZE - 1;
	ht->fill = 0;
	ht->used = 0;
	ht->table = genht_calloc(ht, ht->mask + 1, sizeof(HT(entry_t)));
	if (!ht->table)
		return -1;
	ht->keyhash = keyhash;
	ht->keyeq = keyeq;
	return 0;
}

void HT(uninit)(HT(t) *ht) {
	genht_free(ht, ht->table);
	ht->table = NULL;
}

HT(t) *HT(alloc)(unsigned int (*keyhash)(HT(const_key_t)), int (*keyeq)(HT(const_key_t), HT(const_key_t))) {
	HT(t) *ht = genht_malloc(NULL, sizeof(HT(t)));

	if (!ht)
		return 0;
	HT(init)(ht, keyhash, keyeq);
	return ht;
}

void HT(clear)(HT(t) *ht) {
	HT(uninit)(ht);
	HT(init)(ht, ht->keyhash, ht->keyeq);
}

void HT(free)(HT(t) *ht) {
	HT(uninit)(ht);
	genht_free(NULL, ht);
}

/* one lookup function to rule them all */
static HT(entry_t) *lookup(HT(t) *ht, HT(const_key_t) key, unsigned int hash) {
	unsigned int mask = ht->mask;
	unsigned int i = hash;
	unsigned int j;
	HT(entry_t) *table = ht->table;
	HT(entry_t) *entry = table + (i & mask);
	HT(entry_t) *free_entry; /* first deleted entry for insert */

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

/* for copy and resize: no deleted entries in ht, the lookedup key is not in ht */
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

HT(t) *HT(copy)(const HT(t) *ht) {
	HT(t) *newht;
	HT(entry_t) *entry;
	unsigned int used = ht->used;

	newht = genht_malloc(NULL, sizeof(HT(t)));
	if (!newht)
		return 0;
	*newht = *ht;
	newht->fill = used;
	newht->table = genht_calloc(ht, newht->mask + 1, sizeof(HT(entry_t)));
	if (!newht->table) {
		genht_free(NULL, newht);
		return 0;
	}
	for (entry = ht->table; used > 0; entry++)
		if (HT(isused)(entry)) {
			used--;
			*cleanlookup(newht, entryhash(entry)) = *entry;
		}
	return newht;
}

int HT(resize)(HT(t) *ht, unsigned int hint) {
	unsigned int newsize;
	unsigned int used = ht->used;
	HT(entry_t) *oldtable = ht->table;
	HT(entry_t) *entry;

	if (hint < used << 1)
		hint = used << 1;
	if (hint > HT_MAXSIZE)
		hint = HT_MAXSIZE;
	for (newsize = HT_MINSIZE; newsize < hint; newsize <<= 1);
	ht->table = genht_calloc(ht, newsize, sizeof(HT(entry_t)));
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
	genht_free(ht, oldtable);
	return 0;
}

int HT(has)(HT(t) *ht, HT(const_key_t) key) {
	HT(entry_t) *entry = lookup(ht, key, ht->keyhash(key));

	return HT(isused)(entry);
}

HT(value_t) HT(get)(HT(t) *ht, HT(const_key_t) key) {
	HT(entry_t) *entry = lookup(ht, key, ht->keyhash(key));

	return HT(isused)(entry) ? entry->value : HT_INVALID_VALUE;
}

HT(entry_t) *HT(getentry)(HT(t) *ht, HT(const_key_t) key) {
	HT(entry_t) *entry = lookup(ht, key, ht->keyhash(key));

	return HT(isused)(entry) ? entry : NULL;
}

/* fill threshold = 3/4 */
static GENHT_INLINE void checkfill(HT(t) *ht) {
	if (ht->fill > ht->mask - (ht->mask >> 2) || ht->fill > ht->used << 2)
		HT(resize)(ht, ht->used << (ht->used > 1 << 16 ? 1 : 2));
}

HT(entry_t) *HT(insert)(HT(t) *ht, HT(key_t) key, HT(value_t) value) {
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

void HT(set)(HT(t) *ht, HT(key_t) key, HT(value_t) value) {
	HT(entry_t) *entry = HT(insert)(ht, key, value);

	if (entry)
		entry->value = value;
}

HT(value_t) HT(pop)(HT(t) *ht, HT(const_key_t) key) {
	HT(entry_t) *entry = lookup(ht, key, ht->keyhash(key));
	HT(value_t) v;

	if (!HT(isused)(entry))
		return HT_INVALID_VALUE;
	ht->used--;
	v = entry->value;
	setdeleted(entry);
	return v;
}

HT(entry_t) *HT(popentry)(HT(t) *ht, HT(const_key_t) key) {
	HT(entry_t) *entry = lookup(ht, key, ht->keyhash(key));

	if (HT(isused)(entry)) {
		ht->used--;
		setdeleted(entry);
		return entry;
	}
	return NULL;
}

void HT(delentry)(HT(t) *ht, HT(entry_t) *entry) {
	if (!HT(isused)(entry))
		return;
	ht->used--;
	setdeleted(entry);
}

#undef HT_INVALID_VALUE
