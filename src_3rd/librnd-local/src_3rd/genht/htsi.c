#include "htsi.h"
#define HT(x) htsi_ ## x
#include "ht.c"
#undef HT
