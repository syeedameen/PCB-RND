/*
    scconfig - sul - software utility libraries
    Copyright (C) 2017  Tibor Palinkas

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

int find_sul_pcre(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "#include <stdlib.h>"
		NL "#include <pcre.h>"
		NL "int main()"
		NL "{"
		NL "	const char *err;"
		NL "	int erro, ret;"
		NL "	pcre *p = pcre_compile(\"fo+b\", 0, &err, &erro, NULL);"
		NL "	ret = pcre_exec(p, NULL, \"hi foobar\", 9, 0, 0, &erro, 1);"
		NL "	if (ret == 0)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	const char *node = "libs/sul/pcre";

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for pcre... ");

	if (try_icl_pkg_config(logdepth, "libs/sul/pcre", test_c, NULL, "libpcre", NULL))
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/include/pcre", "-lpcre"))
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/include/pcre", "-lpcre3"))
		return 0;

	return try_fail(logdepth, node);
}
