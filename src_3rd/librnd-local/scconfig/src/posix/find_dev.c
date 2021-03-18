/*
    scconfig - detection of POSIX library features: special files in /dev
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

static int find_posix_dev_(const char* name, int logdepth, int fatal, const char* const dev_name)
{
	static const char* test_c_template =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	FILE* file = fopen(\"/dev/%s\", \"r\");"
		NL "	if(NULL!=file)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	char test_c[1024];
	char key[128];
	sprintf(test_c, test_c_template, dev_name);
	sprintf(key, "libs/posix/dev/%s", dev_name);

	require("cc/cc", logdepth, fatal);

	report("Checking for /dev/%s... ", dev_name);
	logprintf(logdepth, "find_dev_file: trying to find /dev/%s...\n", dev_name);
	logdepth++;

	if (try_icl(logdepth, key, test_c, NULL, NULL, NULL)) return 0;
	return try_fail(logdepth, key);
}

int find_posix_devnull(const char *name, int logdepth, int fatal)
{
	return find_posix_dev_(name, logdepth, fatal, "null");
}

int find_posix_devzero(const char *name, int logdepth, int fatal)
{
	return find_posix_dev_(name, logdepth, fatal, "zero");
}

int find_posix_devrandom(const char *name, int logdepth, int fatal)
{
	return find_posix_dev_(name, logdepth, fatal, "random");
}

int find_posix_devurandom(const char *name, int logdepth, int fatal)
{
	return find_posix_dev_(name, logdepth, fatal, "urandom");
}
