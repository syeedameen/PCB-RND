/*
    scconfig - sul - software utility libraries
    Copyright (C) 2018  Tibor Palinkas

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

int find_sul_freetype2(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "#include <stdio.h>"
		NL "#include <ft2build.h>"
		NL "#include <freetype.h>"
		NL "int main(int argc, char *argv[])"
		NL "{"
		NL "	FT_Library library;"
		NL "	FT_Error errnum;"
		NL "	errnum = FT_Init_FreeType(&library);"
		NL "	if (errnum == 0)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	const char *node = "libs/sul/freetype2";

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for freetype2... ");

	/* for the case user prefix is supplied */
	if (try_icl(logdepth, node, test_c, NULL, NULL, "-lfreetype"))
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/include/freetype2 -I/usr/include/freetype2/freetype", "-lfreetype"))
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/include/freetype2", "-lfreetype"))
		return 0;

	return try_fail(logdepth, node);
}
