#ifndef GENHT_HTPI_H
#define GENHT_HTPI_H

#define HT_HAS_CONST_KEY
typedef void *htpi_key_t;
typedef const void *htpi_const_key_t;
typedef int htpi_value_t;
#define HT(x) htpi_ ## x
#include "ht.h"
#undef HT

#endif
