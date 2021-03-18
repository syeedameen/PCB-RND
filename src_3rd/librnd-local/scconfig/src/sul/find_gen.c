/*
    scconfig - sul - software utility libraries - repo.hu gen* minilibs
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

int find_sul_genht(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdlib.h>"
		NL "#include <genht/hash.h>"
		NL "#include <genht/htsp.h>"
		NL
		NL "int main(int argc, char *argv[])"
		NL "{"
		NL "	htsp_t *ht;"
		NL "	ht = htsp_alloc(strhash, strkeyeq);"
		NL "	if (ht != NULL)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	const char *node = "libs/sul/genht";

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for genht... ");
	logdepth++;

	if (try_icl(logdepth, node, test_c, NULL, "", "/usr/lib/genht_std.a") != 0)
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "", "-lgenht") != 0)
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/local/include", "/usr/local/lib/genht_std.a") != 0)
		return 0;

	return try_fail(logdepth, node);
}

int find_sul_genhvbox(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdlib.h>"
		NL "#include <genhvbox/hvbox.h>"
		NL
		NL "int main(int argc, char *argv[])"
		NL "{"
		NL "	hvbox_t *b = hvbox_new_root(16, 16, NULL);"
		NL "	if (b != NULL)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	const char *node = "libs/sul/genhvbox";

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for genhvbox... ");
	logdepth++;

	if (try_icl(logdepth, node, test_c, NULL, "", "/usr/lib/genhvbox.a") != 0)
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "", "-lgenhvbox") != 0)
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/local/include", "/usr/local/lib/genhvbox.a") != 0)
		return 0;

	return try_fail(logdepth, node);
}

int find_sul_genvector(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdlib.h>"
		NL "#include <genvector/gds_char.h>"
		NL
		NL "int main(int argc, char *argv[])"
		NL "{"
		NL "	gds_t s;"
		NL "	gds_init(&s);"
		NL "	gds_append_str(&s, \"hello\");"
		NL "	if (gds_len(&s) == 5)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	const char *node = "libs/sul/genvector";

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for genvector... ");
	logdepth++;

	if (try_icl(logdepth, node, test_c, NULL, "", "/usr/lib/genvector.a") != 0)
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "", "-lgenvector") != 0)
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/local/include", "/usr/local/lib/genvector.a") != 0)
		return 0;

	return try_fail(logdepth, node);
}

int find_sul_gentrex(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdlib.h>"
		NL "#include <gentrex/helper_intop.h>"
		NL
		NL "int main(int argc, char *argv[])"
		NL "{"
		NL "	if (intop_parse(\">=\") == INTOP_GTE)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	const char *node = "libs/sul/gentrex";

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for gentrex... ");
	logdepth++;

	if (try_icl(logdepth, node, test_c, NULL, "", "-lgentrex") != 0)
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "", "/usr/lib/libgentrex.a") != 0)
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/local/include", "/usr/local/lib/libgentrex.a") != 0)
		return 0;

	return try_fail(logdepth, node);
}

int find_sul_genusin(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdlib.h>"
		NL "#include <genusin.h>"
		NL
		NL "#ifndef GENUSIN_SUPPORTS_API_V2"
		NL "#error Need a genusin version that supports API V2"
		NL "#endif"
		NL
		NL "int main(int argc, char *argv[])"
		NL "{"
		NL "	char *s = genusin_strclone(\"hello\");"
		NL "	if ((s != NULL) && (s[0] == 'h') && (s[1] == 'e'))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	const char *node = "libs/sul/genusin";

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for genusin... ");
	logdepth++;

	if (try_icl(logdepth, node, test_c, NULL, "", "") != 0)
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/local/include", "") != 0)
		return 0;

	return try_fail(logdepth, node);
}

int find_sul_genregex(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdlib.h>"
		NL "#include <string.h>"
		NL "#include <genregex/regex.h>"
		NL
		NL "int main(int argc, char *argv[])"
		NL "{"
		NL "	const char *s = re_error_str(RE_ERR_CLOBADNFA);"
		NL "	if ((s != NULL) && (strlen(s) > 2))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	const char *node = "libs/sul/genregex";

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for genregex... ");
	logdepth++;

	if (try_icl(logdepth, node, test_c, NULL, "", "-lgenregex") != 0)
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/local/include", "-lgenregex") != 0)
		return 0;

	return try_fail(logdepth, node);
}

int find_sul_genlist(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdlib.h>"
		NL "#include <genlist/genadlist.h>"
		NL
		NL "int main(int argc, char *argv[])"
		NL "{"
		NL "	gadl_list_t *l = gadl_list_init(NULL, 16, NULL, NULL);"
		NL "	if (l != NULL)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	const char *node = "libs/sul/genlist";

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for genlist... ");
	logdepth++;

	if (try_icl(logdepth, node, test_c, NULL, "", "-lgenlist") != 0)
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/local/include", "-lgenlist") != 0)
		return 0;

	return try_fail(logdepth, node);
}
