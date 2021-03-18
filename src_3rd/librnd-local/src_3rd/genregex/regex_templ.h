#include "regex.h"

#if RE_BIN_API == 1
#define SRCLEN , int srclen
#define PATLEN , int patlen
#else
#define SRCLEN
#define PATLEN
#endif

typedef struct RE_PRFX(s) RE_PRFX(t);

RE_PRFX(t) *RE_PRFX(comp)(const char *pat);
extern int RE_PRFX(exec)(RE_PRFX(t) *re, const char *str SRCLEN);
extern void RE_PRFX(modw)(RE_PRFX(t) *re, char *wpat);
extern int RE_PRFX(backref)(RE_PRFX(t) *re, char **dst, const char *src SRCLEN);
extern int RE_PRFX(subst)(RE_PRFX(t) *r, char **dst, const char *src SRCLEN, const char *pat PATLEN, int first_only);
extern int RE_PRFX(tag)(RE_PRFX(t) *re, int tagid, char **begin, char **end);
extern void RE_PRFX(free)(RE_PRFX(t) *re);
extern re_error_t RE_PRFX(errno)(RE_PRFX(t) *re);
extern int RE_PRFX(has_error)(RE_PRFX(t) *re);

#undef SRCLEN
#undef PATLEN
