/* Extended regex with string API */
#define RE_PRFX(name) re_be_ ## name
#define RE_BIN_API 1
#include "regex_templ.h"
#undef RE_BIN_API
#undef RE_PRFX
