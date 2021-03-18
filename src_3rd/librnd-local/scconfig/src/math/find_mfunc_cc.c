/*
    scconfig - math func corner case detection
    Copyright (C) 2012  Tibor Palinkas

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
#include <ctype.h>
#include <unistd.h>
#include "libs.h"
#include "log.h"
#include "db.h"
#include "dep.h"

static char *test_c_templ =
	NL "#include <stdio.h>"
	NL "#include <math.h>"
	NL "#include <errno.h>"
	NL "int main(int argc, char *argv[]) {"
	NL "	double magic[] = { %s };"
	NL "	errno = 0;\n"
	NL "	printf(\"%%f:%%d\\n\", %s(magic[atoi(argv[1])]), errno);"
	NL "	return 0;"
	NL "}"
	NL;

/* returns 0 if couldn't run (emu), -1 on error and 1 when got enough results */
static int try(int logdepth, const char *func, const char *values, char **res, int num_res)
{
	char *test_c, *fn = NULL, *fn_esc, *cmd, *out;
	int n, len;

	len = strlen(test_c_templ) + strlen(func) + strlen(values) + 1;
	test_c = malloc(len);
	sprintf(test_c, test_c_templ, values, func);

	logprintf(logdepth, "testing '%s' on '%s'\n", func, values);

	if (compile_code(logdepth+1, test_c, &fn, NULL, NULL, "-lm") != 0) {
		free(test_c);
		return -1;
	}
	free(test_c);

	if (isblind(db_cwd)) {
		/* assume corner cases passed as we have nothing better to do */
		for(n = 0; n < num_res; n++)
			res[n] = strclone("unknown");
		unlink(fn);
		free(fn);
		return 0;
	}

	fn_esc = shell_escape_dup(fn);
	cmd = malloc(strlen(fn_esc) + 32);
	for(n = 0; n < num_res; n++) {
		sprintf(cmd, "%s %d", fn_esc, n);
		if (run(logdepth+1, cmd, &out) == 0) {
			char *s, *dot = NULL;
			int is_int = 1;
			for(s = out; *s != '\0'; s++) {
				switch(*s) {
					case '\n':
					case '\r':
						*s = '\0';
						goto at_end;
					case '.':
						dot = s;
						break;
					default:
						if ((dot != NULL) && (*s != '0'))
							is_int = 0;
						*s = tolower(*s);
				}
			}
			at_end:;
			if ((is_int) && (dot != NULL))
				*dot = '\0';
			if (*out == '+')
				res[n] = strclone(out+1);
			else
				res[n] = strclone(out);
			free(out);
		}
		else
			res[n] = strclone("fpe");
	}
	free(cmd);
	free(fn_esc);
	unlink(fn);
	free(fn);
	return 1;
}


static int find_math_cc(const char *name, int logdepth, int fatal, char *func, char *inp, char **node_names, int num_tests)
{
	char *res[256];
	char node_name[256];
	int n, ret;

	ret = 0;
	require("cc/cc", logdepth, fatal);

	report("Checking for %s() corner cases... ", func);
	logprintf(logdepth, "find_math_cc_log: Checking for %s() corner cases... \n", func);
	logdepth++;

	if (try(logdepth, func, inp, res, num_tests) >= 0) {
		for(n = 0; n < num_tests; n++) {
			char *sep;
			report(".");
			sep = strchr(res[n], ':');
			if (sep != NULL) {
				*sep = 0;
				sep++;
				sprintf(node_name, "libs/math/cc/%s/%s/return", func, node_names[n]);
				put(node_name, res[n]);
				sprintf(node_name, "libs/math/cc/%s/%s/errno", func, node_names[n]);
				put(node_name, sep);
			}
			else
				ret = 1;
		}
	}

	/* avoid redetection */
	sprintf(node_name, "libs/math/cc/%s/presents", func);
	put(node_name, strue);


	report(ret ? "done with errors\n" : " done.\n");
	return ret;
}

int find_math_cc_log(const char *name, int logdepth, int fatal)
{
	char *inp     = "+0.0, -0.0, 1.0, -1.0, 1/0.0";
	char *nodes[] = {"p_0", "m_0", "p_1", "m_1", "p_inf"};
	return find_math_cc(name, logdepth, fatal, "log", inp, nodes, sizeof(nodes) / sizeof(char *));
}
