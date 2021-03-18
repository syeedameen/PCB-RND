#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "htsi.h"

static unsigned int keyhash(char *key) {
	unsigned char *p = (unsigned char *)key;
	unsigned int hash = 0;

	while (*p)
		hash += (hash << 2) + *p++;
	return hash;
}

static int keyeq(char *a, char *b) {
	char *pa = (char *)a;
	char *pb = (char *)b;

	for (; *pa == *pb; pa++, pb++)
		if (*pa == '\0')
			return 1;
	return 0;
}

int main() {
	htsi_t *ht;
	htsi_entry_t *e;

	ht = htsi_alloc(keyhash, keyeq);
	htsi_set(ht, "a", 1);
	htsi_set(ht, "b", 2);
	htsi_set(ht, "asdf", -3);
	htsi_set(ht, "qw", 4);
	htsi_set(ht, "v", 5);
	htsi_set(ht, "df", 6);
	htsi_set(ht, "x", 7);
	if (!htsi_has(ht, "a"))
		puts("ERR: has a");
	if (htsi_has(ht, "1"))
		puts("ERR: has 1");
	if (htsi_insert(ht, "y", 8))
		puts("ERR: insert y");
	if (htsi_insert(ht, "x", 9)->value != 7)
		puts("ERR: insert x");
	if (htsi_pop(ht, "b") != 2 || htsi_getentry(ht, "b"))
		puts("ERR: pop b");
	if (htsi_popentry(ht, "b"))
		puts("ERR: pope b");
	if (htsi_popentry(ht, "c"))
		puts("ERR: pope c");
	for (e = htsi_first(ht); e; e = htsi_next(ht, e)) {
		if (htsi_get(ht, e->key) != e->value)
			printf("ERR %s %d\n", e->key, e->value);
		printf("%s %d\n", e->key, e->value);
	}
	htsi_clear(ht);
	for (e = htsi_first(ht); e; e = htsi_next(ht, e))
		puts("ERR: clear");
	htsi_free(ht);
	return 0;
}
