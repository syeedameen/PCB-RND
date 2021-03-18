/*
    scconfig - detection of POSIX library features: resource specific calls
    Copyright (C) 2018  Tibor Palinkas
    Copyright (C) 2019  Aron Barath

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

int find_posix_openlog(const char *name, int logdepth, int fatal)
{
	const char *key = "libs/posix/log/openlog";
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main()"
		NL "{"
		NL "	openlog(\"scconfig\", LOG_PID, LOG_USER);"
		NL "	puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for openlog()... ");
	logprintf(logdepth, "find_posix_openlog: trying to find openlog()...\n");
	logdepth++;

	if (try_icl(logdepth, key, test_c, "#include <syslog.h>", NULL, NULL))
		return 0;

	return try_fail(logdepth, key);
}

int find_posix_syslog(const char *name, int logdepth, int fatal)
{
	const char *key = "libs/posix/log/syslog";
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main()"
		NL "{"
		NL "	syslog(LOG_DEBUG, \"scconfig test message\");"
		NL "	puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for syslog()... ");
	logprintf(logdepth, "find_posix_syslog: trying to find syslog()...\n");
	logdepth++;

	if (try_icl(logdepth, key, test_c, "#include <syslog.h>", NULL, NULL))
		return 0;

	return try_fail(logdepth, key);
}

int find_posix_vsyslog(const char *name, int logdepth, int fatal)
{
	const char *key = "libs/posix/log/vsyslog";
	const char *test_c =
		NL "#include <stdio.h>"
		NL "void test_vsyslog(const char *fmt, ...)"
		NL "{"
		NL "	va_list ap;"
		NL "	va_start(ap, fmt);"
		NL "	vsyslog(LOG_DEBUG, fmt, ap);"
		NL "	va_end(ap);"
		NL "}"
		NL "int main()"
		NL "{"
		NL "	test_vsyslog(\"scconfig %s\", \"test message\");"
		NL "	puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for vsyslog()... ");
	logprintf(logdepth, "find_posix_vsyslog: trying to find vsyslog()...\n");
	logdepth++;

	if (try_icl(logdepth, key, test_c, "#include <syslog.h>\n#include <stdarg.h>", NULL, NULL))
		return 0;

	return try_fail(logdepth, key);
}
