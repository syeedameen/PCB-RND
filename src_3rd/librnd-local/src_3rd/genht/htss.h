#ifndef GENHT_HTSS_H
#define GENHT_HTSS_H

#define HT_HAS_CONST_KEY
typedef char *htss_key_t;
typedef const char *htss_const_key_t;
typedef char *htss_value_t;
#define HT(x) htss_ ## x
#include "ht.h"
#undef HT

#endif
