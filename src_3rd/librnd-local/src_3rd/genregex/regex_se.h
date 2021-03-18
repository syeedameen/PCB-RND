/* Extended regex with string API */
#define RE_PRFX(name) re_se_ ## name
#include "regex_templ.h"
#undef RE_PRFX
