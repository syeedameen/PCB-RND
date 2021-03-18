#include <stddef.h>
#include <stdint.h>
/* hash (in,len) using 16byte secret k so collision attacks are hard */
uint64_t siphash(const uint8_t *in, size_t len, const uint8_t *k);
