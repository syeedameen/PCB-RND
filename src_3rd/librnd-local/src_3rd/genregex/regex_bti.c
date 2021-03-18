/* generalized regex lib based on Ozan S. Yigit's implementation.
   Generalization done by  Tibor 'Igor2' Palinkas in 2012
   This file is placed in the Public Domain.
*/

#define RE_PRFX(name) re_bti_ ## name
#define RE_EXTEND 0
#define RE_BIN_API 1
#define RE_ICASE 1

#include "regex_templ.c"

