#ifndef GENHT_HTSP_H
#define GENHT_HTSP_H

#define HT_HAS_CONST_KEY
typedef char *htsp_key_t;
typedef const char *htsp_const_key_t;
typedef void *htsp_value_t;
#define HT(x) htsp_ ## x
#include "ht.h"
#undef HT

#endif
