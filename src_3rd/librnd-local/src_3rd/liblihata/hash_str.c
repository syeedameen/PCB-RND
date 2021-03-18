#include <string.h>

int lht_str_keyeq(const char *k1, const char *k2)
{
	return !strcmp(k1, k2);
}

/* simple string hash */
#define SEED 0x9e3779b9
unsigned lht_str_keyhash(const char *key) {
	unsigned char *p = (unsigned char *)key;
	unsigned h = SEED;

	while (*p)
		h += (h << 2) + *p++;
	return h;
}
