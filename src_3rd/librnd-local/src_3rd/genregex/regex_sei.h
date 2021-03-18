/* Extended regex with string API, case insensitive */
#define RE_PRFX(name) re_sei_ ## name
#include "regex_templ.h"
#undef RE_PRFX
