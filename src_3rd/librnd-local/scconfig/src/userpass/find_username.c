/*
    scconfig - user name related API detection
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
#include <string.h>
#include "libs.h"
#include "log.h"
#include "db.h"
#include "dep.h"

static int find_username_getpw(const char *name, int logdepth, int fatal, const char *call, const char *arg)
{
	char *test_c_ =
		NL "#include <stdlib.h>"
		NL "int main() {"
		NL "	struct passwd *p;"
		NL "	p = %s(%s);"
		NL "	if (p != NULL)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	char test_c[512];
	char node[128];

	char *inc_default =
		NL "#include <sys/types.h>"
		NL "#include <pwd.h>"
		NL;

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for %s... ", call);
	logprintf(logdepth, "find_username_getpw: trying to find %s()...\n", call);
	logdepth++;

	sprintf(test_c, test_c_, call, arg);
	sprintf(node, "libs/userpass/%s", call);
	if (try_icl(logdepth, node, test_c, inc_default, NULL, NULL) == 0)
		return try_fail(logdepth, node);
	return 0;
}

int find_username_getpwuid(const char *name, int logdepth, int fatal)
{
	return find_username_getpw(name, logdepth, fatal, "getpwuid", "0");
}

int find_username_getpwnam(const char *name, int logdepth, int fatal)
{
	return find_username_getpw(name, logdepth, fatal, "getpwnam", "\"root\"");
}
