#ifndef VTII_H
#define VTII_H

#include <stdlib.h>
#include <string.h>

/* Elem=int; init=function
   int vector, there's an init function for the newly allocated elements (but no
   memory cleaning other than that); useful if new elements need to be set
   to some non-zero initial value (e.g. -1). Set up function pointer
   ->init_elem  */

/* For documentation on the settings, check vti0.h */

#define GVT(x) vtii_ ## x
#define GVT_ELEM_TYPE int
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 4096
#define GVT_START_SIZE 32
#define GVT_FUNC
#define GVT_INIT_ELEM_FUNC

#include <genvector/genvector_impl.h>

#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)

#include <genvector/genvector_undef.h>

#endif
