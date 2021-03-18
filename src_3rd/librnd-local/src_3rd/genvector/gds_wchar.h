#ifndef GDS_CHAR_H
#define GDS_CHAR_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <wchar.h>

/* all public symbols are wrapped in GDS() */
#define GVT(x) wgds_ ## x

/* Character type - a string consists of elements of this type */
#define GVT_ELEM_TYPE wchar_t

/* Type that represents string lengths */
#define GVT_SIZE_TYPE size_t

/* Below this length, always double allocation size when the string grows */
#define GVT_DOUBLING_THRS 1024

/* Initial string size when the first element is written */
#define GVT_START_SIZE 32

/* Optional terminator; when present, it is always appended at the end */
#define GVT_TERM ((wchar_t)(0))

/* Optional strlen(); if there's no strlen and there's GVT_TERM, append()
   implements a loop to determine input string length. If neither of these
   is #defined, there's no append(). */

#if __STDC_VERSION__ >= 199901L
#	define GVT_STRLEN(s) wcslen(s)
#else
/* implement a loop becuase GVT_TERM is defined */
#endif

/* Optional prefix for function definitions (e.g. static inline) */
#define GVT_FUNC

/* An extra no_realloc field; when it is set to non-zero by the user, no
   realloc() is called (any attempt to grow the array fails) */
#define GVT_OPTIONAL_NO_REALLOC

/* Include the actual header implementation */
#include <genvector/genvector_impl.h>

/* Memory allocator */
#ifndef GVT_WCHAR_ALLOC
#	define GVT_REALLOC(gds, ptr, size) realloc(ptr, size)
#else
#	define GVT_REALLOC(gds, ptr, size) GVT_WCHAR_REALLOC(gds, ptr, size)
#endif

#ifndef GVT_WCHAR_FREE
#	define GVT_FREE(gds, ptr) free(ptr)
#else
#	define GVT_FREE(gds, ptr) GVT_WCHAR_FREE(gds, ptr)
#endif

/* clean up #defines */
#include <genvector/genvector_undef.h>

/* rename macros for convenience */
#define wgds_append_str  wgds_append_array
#define wgds_insert_str(vect, from, str)  wgds_insert_len((vect), (from), (str), wcslen(str))
#define wgds_prepend_len(vect, ptr, len) wgds_insert_len((vect), 0, (ptr), (len))
#define wgds_prepend_str(vect, str) wgds_insert_len((vect), 0, (str), wcslen(str))

#endif
