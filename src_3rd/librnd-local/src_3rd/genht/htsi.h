#ifndef GENHT_HTSI_H
#define GENHT_HTSI_H

#define HT_HAS_CONST_KEY
typedef char *htsi_key_t;
typedef const char *htsi_const_key_t;
typedef int htsi_value_t;
#define HT(x) htsi_ ## x
#include "ht.h"
#undef HT

#endif
