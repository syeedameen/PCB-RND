#ifndef GENHT_HT_UTILS_H
#define GENHT_HT_UTILS_H

/* iterate htent over all entries of ht, executing uninit_code on each and
   the uninit the hash table. Example call:

   genht_uninit_deep(htsp, &my_tasble, {
     free(htent->key);
     free(htent->value);
   });
*/
#define genht_uninit_deep(type, ht, uninit_code) \
do { \
	type ## _entry_t *htent; \
	for(htent = type ## _first(ht); htent != NULL; htent = type ## _next(ht, htent)) { uninit_code; } \
	type ## _uninit(ht); \
} while(0)

#endif
