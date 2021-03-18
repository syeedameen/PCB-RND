/*
    scconfig - detection of POSIX library features: resource specific calls
    Copyright (C) 2018  Tibor Palinkas
    Copyright (C) 2018  Aron Barath

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

int find_posix_getrusage(const char *name, int logdepth, int fatal)
{
	const char *key = "libs/posix/res/getrusage";
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main()"
		NL "{"
		NL "	struct rusage usage;"
		NL "	if (getrusage(RUSAGE_SELF, &usage) == 0)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for getrusage()... ");
	logprintf(logdepth, "find_posix_getrusage: trying to find getrusage()...\n");
	logdepth++;

	if (try_icl(logdepth, key, test_c, "#include <sys/resource.h>", NULL, NULL))
		return 0;
	if (try_icl(logdepth, key, test_c, "#include <sys/time.h>\n#include <sys/resource.h>", NULL, NULL))
		return 0;

	return try_fail(logdepth, key);
}

int find_posix_clockgettime(const char *name, int logdepth, int fatal)
{
	const char *key = "libs/posix/res/clock_gettime";
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main()"
		NL "{"
		NL "	struct timespec t;"
		NL "	if (clock_gettime(CLOCK_REALTIME, &t) == 0)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for clock_gettime()... ");
	logprintf(logdepth, "find_posix_clockgettime: trying to find clock_gettime()...\n");
	logdepth++;

	if (try_icl(logdepth, key, test_c, "#include <time.h>", NULL, NULL))
		return 0;
	if (try_icl(logdepth, key, test_c, "#include <time.h>", NULL, "-lrt"))
		return 0;

	return try_fail(logdepth, key);
}
