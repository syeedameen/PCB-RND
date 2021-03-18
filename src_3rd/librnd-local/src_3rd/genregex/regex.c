/* generalized regex lib based on Ozan S. Yigit's implementation.
   Generalization done by  Tibor 'Igor2' Palinkas in 2012
   This file is placed in the Public Domain.
*/

#include "regex.h"

static const char *msg[] = {
	"success",
	"No previous regular expression",
	"Missing ]",
	"Empty closure",
	"Illegal closure before + or *",
	"Too many \\(\\) pairs",
	"Null pattern inside \\(\\)",
	"Unmatched \\)",
	"Null pattern inside \\<\\>",
	"Cyclical reference",
	"Undetermined reference",
	"Unmatched \\(",
	"closure: bad nfa.",
	"re_exec: bad nfa."
};

const char *re_error_str(re_error_t code)
{
	if ((code < 0) || (code >= (sizeof(msg) / sizeof(char *))))
		return "Unknown/invalid error";
	return msg[code];
}
