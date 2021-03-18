/*
    scconfig - math feature detection
    Copyright (C) 2013  Tibor Palinkas

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
#include <unistd.h>
#include "libs.h"
#include "log.h"
#include "db.h"
#include "dep.h"
#include "find_mfunc_cc.h"

#define MATHH "#include <math.h>"
int find_math_isnan(const char *name, int logdepth, int fatal)
{
	char *test_c =
		NL "int main() {"
		NL "	if (!isnan(1.0))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for isnan... ");
	logprintf(logdepth, "find_math_isnan: trying to find isnan...\n");
	logdepth++;

	if (try_icl(logdepth, "libs/math/isnan", test_c, MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isnan", test_c, MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, "libs/math/isnan", test_c, "#define _BSD_SOURCE\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isnan", test_c, "#define _BSD_SOURCE\n" MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, "libs/math/isnan", test_c, "#define _XOPEN_SOURCE\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isnan", test_c, "#define _XOPEN_SOURCE\n" MATHH, NULL, "-lm")) return 0;

	return try_fail(logdepth, "libs/math/isnan");
}

int find_math_isinf(const char *name, int logdepth, int fatal)
{
	char *test_c =
		NL "int main() {"
		NL "	if (!isinf(1.0))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for isinf... ");
	logprintf(logdepth, "find_math_isinf: try_icling to find isinf...\n");
	logdepth++;

	if (try_icl(logdepth, "libs/math/isinf", test_c, MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isinf", test_c, MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, "libs/math/isinf", test_c, "#define _BSD_SOURCE\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isinf", test_c, "#define _BSD_SOURCE\n" MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, "libs/math/isinf", test_c, "#define _XOPEN_SOURCE\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isinf", test_c, "#define _XOPEN_SOURCE\n" MATHH, NULL, "-lm")) return 0;

	return try_fail(logdepth, "libs/math/isinf");
}


int find_math_isfinite(const char *name, int logdepth, int fatal)
{
	char *test_c =
		NL "int main() {"
		NL "	if (isfinite(1.0))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for isfinite... ");
	logprintf(logdepth, "find_math_isfinite: try_icling to find isfinite...\n");
	logdepth++;

	if (try_icl(logdepth, "libs/math/isfinite", test_c, MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isfinite", test_c, MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, "libs/math/isfinite", test_c, "#define _BSD_SOURCE\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isfinite", test_c, "#define _BSD_SOURCE\n" MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, "libs/math/isfinite", test_c, "#define _XOPEN_SOURCE\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isfinite", test_c, "#define _XOPEN_SOURCE\n" MATHH, NULL, "-lm")) return 0;

	return try_fail(logdepth, "libs/math/isfinite");
}


int find_math_isnormal(const char *name, int logdepth, int fatal)
{
	char *test_c =
		NL "int main() {"
		NL "	if (isnormal(1.0))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for isnormal... ");
	logprintf(logdepth, "find_math_isnormal: try_icling to find isnormal...\n");
	logdepth++;

	if (try_icl(logdepth, "libs/math/isnormal", test_c, MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isnormal", test_c, MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, "libs/math/isnormal", test_c, "#define _BSD_SOURCE\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isnormal", test_c, "#define _BSD_SOURCE\n" MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, "libs/math/isnormal", test_c, "#define _XOPEN_SOURCE\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isnormal", test_c, "#define _XOPEN_SOURCE\n" MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, "libs/math/isnormal", test_c, "#define _ISOC99_SOURCE\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isnormal", test_c, "#define _ISOC99_SOURCE\n" MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, "libs/math/isnormal", test_c, "#define _XOPEN_SOURCE 600\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/isnormal", test_c, "#define _XOPEN_SOURCE 600\n" MATHH, NULL, "-lm")) return 0;

	return try_fail(logdepth, "libs/math/isnormal");
}


int find_math_nan(const char *name, int logdepth, int fatal)
{
	char *test_c =
		NL "int main() {"
		NL "	if (nan(\"foo\") != 0.0)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for nan... ");
	logprintf(logdepth, "find_math_nan: try_icling to find nan...\n");
	logdepth++;

	if (try_icl(logdepth, "libs/math/nan", test_c, MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/nan", test_c, MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, "libs/math/nan", test_c, "#define _ISOC99_SOURCE\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/nan", test_c, "#define _ISOC99_SOURCE\n" MATHH, NULL, "-lm")) return 0;
	if (try_icl(logdepth, "libs/math/nan", test_c, "#define _XOPEN_SOURCE 600\n" MATHH, NULL, NULL)) return 0;
	if (try_icl(logdepth, "libs/math/nan", test_c, "#define _XOPEN_SOURCE 600\n" MATHH, NULL, "-lm")) return 0;

	return try_fail(logdepth, "libs/math/nan");
}


int find_math_nanop(const char *name, int logdepth, int fatal)
{
	char *test_c_temp =
		NL "#include <stdlib.h>"
		NL "#include <stdio.h>"
		NL "%s"
		NL
		NL "double s2d(const char *s)"
		NL "{"
		NL "	if (strcmp(s, \"nan\") == 0)"
		NL "		return nan(\"nan\");"
		NL "	return atof(s);"
		NL "}"
		NL "int main(int argc, char *argv[])"
		NL "{"
		NL "	double op1, op2, res;"
		NL "	op1 = s2d(argv[1]);"
		NL "	op2 = s2d(argv[3]);"

		NL "	switch(*argv[2]) {"
		NL "		case '+': res = op1 + op2; break;"
		NL "		case '-': res = op1 - op2; break;"
		NL "		case 'M': res = op1 * op2; break;"
		NL "		case '/': res = op1 / op2; break;"
		NL "	}"
		NL "	if (isnan(res))"
		NL "		printf(\"nan\\n\");"
		NL "	else"
		NL "		printf(\"%%f\\n\", res);"
		NL "	return 0;"
		NL "}"
		NL;
	char test_c[2048];
	char *inc, *test_bin, *test_bin_esc, *out, *s, *cmd = test_c;
	char *tests[] = {
		"%s 1 + 1",    "2.0",  NULL,
		"%s nan + 1",  "nan",  "add",
		"%s nan - 1",  "nan",  "sub",
		"%s nan M 1",  "nan",  "multiply",
		"%s nan / 1",  "nan",  "divide",
		NULL, NULL
	};
	char **test;
	int bad = 0;

	require("libs/math/nan/presents", logdepth, fatal);
	require("libs/math/isnan/presents", logdepth, fatal);
	require("cc/cc", logdepth, fatal);
	if (!istrue(get("libs/math/isnan/presents")) || !istrue(get("libs/math/nan/presents")))
		return try_fail(logdepth, "libs/math/nanop/presents");

	inc = esc_interpret(get("libs/math/isnan/includes"));
	sprintf(test_c, test_c_temp, inc);
	free(inc);

	report("Checking for nanop... ");
	logprintf(logdepth, "find_math_nan: try_icling to find nan...\n");
	logdepth++;

	test_bin = NULL;
	if (compile_code(logdepth, test_c, &test_bin, NULL, get("libs/math/nan/cflags"), get("libs/math/nan/ldflags")) != 0)
		return try_fail(logdepth, "libs/math/nanop/presents");
	report("found\n");

	test_bin_esc = shell_escape_dup(test_bin);
	for(test = tests; *test != NULL; test += 3) {
		report(test[0], "");
		report("... ");
		sprintf(cmd, test[0], test_bin_esc);
		run(logdepth, cmd, &out);
		if (test[2] != NULL)
			sprintf(cmd, "libs/math/nanop/%s", test[2]);
		if (out != NULL) {
			if (target_emu_fail(out) || (test[1] == NULL) || (strncmp(out, test[1], strlen(test[1])) == 0)) {
				report("OK (%s)\n", test[1]);
				if (test[2] != NULL)
					put(cmd, test[1]);
			}
			else {
				report("? (");
				for(s = out; *s != '\0'; s++)
					if ((*s == '\n') || (*s == '\r'))
						*s = ' ';
				report(out);
				report(")\n");
				if (test[2] != NULL)
					put(cmd, out);
				bad = 1;
			}
			free(out);
		}
		else
			bad = 1;
	}

	if (bad)
		put("libs/math/nanop/allok", sfalse);
	else
		put("libs/math/nanop/allok", strue);

	put("libs/math/nanop/presents", strue);

	unlink(test_bin);
	free(test_bin);
	free(test_bin_esc);
	return 0;
}

