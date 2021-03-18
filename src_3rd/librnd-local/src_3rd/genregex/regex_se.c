/* generalized regex lib based on Ozan S. Yigit's implementation.
   Generalization done by  Tibor 'Igor2' Palinkas in 2012
   This file is placed in the Public Domain.
*/

#define RE_PRFX(name) re_se_ ## name
#define RE_EXTEND 1
#define RE_BIN_API 0
#define RE_ICASE 0

#include "regex_templ.c"

