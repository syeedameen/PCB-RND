#ifndef GDS_CHAR_H
#define GDS_CHAR_H
#include <stdlib.h>
#include <string.h>

/* all public symbols are wrapped in GDS() */
#define GVT(x) gds_ ## x

/* Character type - a string consists of elements of this type */
#define GVT_ELEM_TYPE char

/* Type that represents string lengths */
#define GVT_SIZE_TYPE size_t

/* Below this length, always double allocation size when the string grows */
#define GVT_DOUBLING_THRS 4096

/* Initial string size when the first element is written */
#define GVT_START_SIZE 32

/* Optional terminator; when present, it is always appended at the end */
#define GVT_TERM '\0'
#define GVT_TERMSIZE 1

/* Optional strlen(); if there's no strlen and there's GVT_TERM, append()
   implements a loop to determine input string length. If neither of these
   is #defined, there's no append(). */
#define GVT_STRLEN(s) strlen(s)

/* Optional prefix for function definitions (e.g. static inline) */
#define GVT_FUNC

/* An extra no_realloc field; when it is set to non-zero by the user, no
   realloc() is called (any attempt to grow the array fails) */
#define GVT_OPTIONAL_NO_REALLOC

/* Include the actual header implementation */
#include <genvector/genvector_impl.h>

/* Memory allocator */
#ifndef GVT_CHAR_ALLOC
#	define GVT_REALLOC(gds, ptr, size) realloc(ptr, size)
#else
#	define GVT_REALLOC(gds, ptr, size) GVT_CHAR_REALLOC(gds, ptr, size)
#endif

#ifndef GVT_CHAR_FREE
#	define GVT_FREE(gds, ptr) free(ptr)
#else
#	define GVT_FREE(gds, ptr) GVT_CHAR_FREE(gds, ptr)
#endif

/* clean up #defines */
#include <genvector/genvector_undef.h>

/* rename macros for convenience */
#define gds_append_str  gds_append_array
#define gds_insert_str(vect, from, str)  gds_insert_len((vect), (from), (str), strlen(str))
#define gds_prepend_len(vect, ptr, len) gds_insert_len((vect), 0, (ptr), (len))
#define gds_prepend_str(vect, str) gds_insert_len((vect), 0, (str), strlen(str))
#endif
