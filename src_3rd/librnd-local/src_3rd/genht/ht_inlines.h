/* helper functions */
GENHT_STATIC GENHT_INLINE unsigned int HT(length)(const HT(t) *ht) {return ht->used;}
GENHT_STATIC GENHT_INLINE unsigned int HT(fill)(const HT(t) *ht) {return ht->fill;}
GENHT_STATIC GENHT_INLINE unsigned int HT(size)(const HT(t) *ht) {return ht->mask + 1;}

/* for any entry exactly one returns true */
GENHT_STATIC GENHT_INLINE int HT(isused)(const HT(entry_t) *entry) {return entry->flag > 0;}
GENHT_STATIC GENHT_INLINE int HT(isempty)(const HT(entry_t) *entry) {return entry->flag == 0;}
GENHT_STATIC GENHT_INLINE int HT(isdeleted)(const HT(entry_t) *entry) {return entry->flag < 0;}

/* first used (useful for iteration) */
GENHT_STATIC GENHT_INLINE HT(entry_t) *HT(first)(const HT(t) *ht)
{
	HT(entry_t) *entry = 0;

	if (ht->used)
		for (entry = ht->table; !HT(isused)(entry); entry++);
	return entry;
}

/* next used (useful for iteration) */
GENHT_STATIC GENHT_INLINE HT(entry_t) *HT(next)(const HT(t) *ht, HT(entry_t) *entry)
{
	while (++entry != ht->table + ht->mask + 1)
		if (HT(isused)(entry))
			return entry;
	return 0;
}
