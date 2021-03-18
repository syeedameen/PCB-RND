/*
    scconfig - math feature detection
    Copyright (C) 2015  Tibor Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

		Project page: http://repo.hu/projects/scconfig
		Contact via email: scconfig [at] igor2.repo.hu
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libs.h"
#include "log.h"
#include "db.h"
#include "dep.h"

#define MATHH "#include <math.h>"

static int test_mathf(const char *name, int logdepth, int fatal, const char *fname, const char *cond)
{
	char *test_c_template =
		NL "float one=1.0, zero=0.0;"
		NL "int main() {"
		NL "	if (%s)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	char test_c[512];
	char node_name[128];

	sprintf(test_c, test_c_template, cond);
	sprintf(node_name, "libs/math/%s", fname);

	require("cc/cc", logdepth, fatal);

	report("Checking for %s... ", fname);
	logprintf(logdepth, "test_mathf: trying to find %s...\n",fname);
	logdepth++;


	if (try_icl(logdepth, node_name, test_c, MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, node_name, test_c, MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, node_name, test_c, "#define _BSD_SOURCE\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, node_name, test_c, "#define _BSD_SOURCE\n" MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, node_name, test_c, "#define _XOPEN_SOURCE\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, node_name, test_c, "#define _XOPEN_SOURCE\n" MATHH, NULL, "-lm")) return 0;

	return try_fail(logdepth, node_name);
}

int find_math_expf(const char *name, int logdepth, int fatal)
{
	return test_mathf(name, logdepth, fatal, "expf", "expf(zero) == 1.0");
}


int find_math_logf(const char *name, int logdepth, int fatal)
{
	return test_mathf(name, logdepth, fatal, "logf", "logf(one) == 0.0");
}

int find_math_rint(const char *name, int logdepth, int fatal)
{
	return test_mathf(name, logdepth, fatal, "rint", "rint(4.0) == 4.0");
}

int find_math_round(const char *name, int logdepth, int fatal)
{
	return test_mathf(name, logdepth, fatal, "round", "round(3.6) == 4.0");
}
