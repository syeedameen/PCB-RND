#ifndef GENHT_HTIP_H
#define GENHT_HTIP_H

typedef long int htip_key_t;
typedef void *htip_value_t;
#define HT(x) htip_ ## x
#include "ht.h"
#undef HT

int htip_keyeq(htip_key_t a, htip_key_t b);

#endif
