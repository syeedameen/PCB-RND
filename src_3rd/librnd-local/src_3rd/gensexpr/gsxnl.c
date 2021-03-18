#include <gensexpr/gsxnl.h>

#define GSX(x) gsxnl_ ## x
#undef GENSEXPR_WANT_LOC

#include <gensexpr/gensexpr_impl.c>
