/*
    scconfig - sul - software utility libraries
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
#include "libs.h"
#include "log.h"
#include "db.h"
#include "dep.h"

int find_sul_glib(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <glib.h>"
		NL "#include <stdlib.h>"
		NL
		NL "int main(int argc, char *argv[])"
		NL "{"
		NL "	GArray *a = g_array_new(FALSE, FALSE, sizeof(char *));"
		NL "	g_array_free(a, FALSE);"
		NL "	puts(\"OK\");"
		NL "	return EXIT_SUCCESS;"
		NL "}"
		NL;
	const char *node = "libs/sul/glib";
	char *cflags;
	char *ldflags;

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for glib... ");
	logprintf(logdepth, "find_glib: running pkg-config...\n");
	logdepth++;

	if (run_pkg_config(logdepth, "glib-2.0", &cflags, &ldflags) != 0)
		return 1;


	if (try_icl(logdepth, node, test_c, NULL, cflags, ldflags) == 0) {
		free(cflags);
		free(ldflags);
		return try_fail(logdepth, node);
	}

	free(cflags);
	free(ldflags);
	return 0;
}
