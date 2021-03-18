/*
    scconfig - sul - software utility libraries
    Copyright (C) 2016  Tibor Palinkas

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

int find_sul_libxml2(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "#include <string.h>"
		NL "#include <libxml/parser.h>"
		NL "char *xml ="
		NL "	\"<?xml version=\\\"1.0\\\" encoding=\\\"UTF-8\\\"?>\""
		NL "	\"<hello/>\";"
		NL "int main()"
		NL "{"
		NL "	xmlDocPtr doc;"
		NL "	doc = xmlReadMemory(xml, strlen(xml), NULL, NULL, 0);"
		NL "	if (doc != NULL)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	const char *node = "libs/sul/libxml2";

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for libxml2... ");

	if (try_icl_pkg_config(logdepth, "libs/sul/libxml2", test_c, NULL, "libxml-*", get("/arg/libxml-version")))
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/include/libxml2", "-lxml2"))
		return 0;

	return try_fail(logdepth, node);
}
