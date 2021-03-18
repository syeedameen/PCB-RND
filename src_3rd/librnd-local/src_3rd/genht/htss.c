#include "htss.h"
#define HT(x) htss_ ## x
#include "ht.c"
#undef HT
