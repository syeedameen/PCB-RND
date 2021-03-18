/*
    scconfig - detection of POSIX library features: file system specific calls
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
#include <unistd.h>
#include "libs.h"
#include "log.h"
#include "db.h"
#include "dep.h"

int find_posix_fallocate(const char *name, int logdepth, int fatal)
{
	char *tmpf;
	char *test_c_in =
		NL "#include <stdio.h>"
		NL "#include <fcntl.h>"
		NL "#include <sys/types.h>"
		NL "int main()"
		NL "{"
		NL "	int fd = open(\"%s\", O_WRONLY);"
		NL "	if (posix_fallocate(fd, 0, 1024) == 0)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	char test_c[8250];

	require("cc/cc", logdepth, fatal);

	report("Checking for posix_fallocate... ");
	logprintf(logdepth, "find_posix_fallocate: trying to find posix_fallocate()...\n");
	logdepth++;

	tmpf = tempfile_new(".psx");
	sprintf(test_c, test_c_in, tmpf);

	if (try_icl(logdepth, "libs/posix/fs/posix_fallocate", test_c, "#define _XOPEN_SOURCE 600\n#include <fcntl.h>", NULL, NULL)) {
		unlink(tmpf);
		free(tmpf);
		return 0;
	}
	unlink(tmpf);
	free(tmpf);
	return try_fail(logdepth, "libs/posix/fs/posix_fallocate");
}

int find_posix_pread(const char *name, int logdepth, int fatal)
{
	const char *node = "libs/posix/fs/pread";
	char *tmpf;
	const char *test_c_template =
		NL "const char testdata[] = \"abcdef\";"
		NL "int try_pread(int fd)"
		NL "{"
		NL "	char buf[2];"
		NL "	if (pread(fd, buf, 2, 3) != 2 ||"
		NL "		buf[0] != testdata[3+0] ||"
		NL "		buf[1] != testdata[3+1])"
		NL "		return 0;"
		NL "	return 1;"
		NL "}"
		NL "#include <stdio.h>"
		NL "#include <sys/types.h>"
		NL "#include <fcntl.h>"
		NL "int main()"
		NL "{"
		NL "	int fd = open(\"%s\", O_RDWR);"
		NL no_implicit(ssize_t, "pread", "pread")
		NL "	if (write(fd, testdata, 6) == 6 &&"
		NL "		try_pread(fd))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	const char* incs[] = {"#include <unistd.h>","#define _XOPEN_SOURCE 600\n#include <unistd.h>",NULL};
	const char** inc;
	char test_c[8250];

	require("cc/cc", logdepth, fatal);

	report("Checking for pread()... ");
	logprintf(logdepth, "find_posix_pread: trying to find pread()...\n");
	logdepth++;

	tmpf = tempfile_new(".psx");
	sprintf(test_c, test_c_template, tmpf);

	for (inc = incs; *inc; ++inc) {
		if (try_icl(logdepth, node, test_c, *inc, NULL, NULL)) {
			unlink(tmpf);
			free(tmpf);
			return 0;
		}
	}
	unlink(tmpf);
	free(tmpf);
	return try_fail(logdepth, node);
}

int find_posix_pwrite(const char *name, int logdepth, int fatal)
{
	const char *node = "libs/posix/fs/pwrite";
	char *tmpf;
	const char *test_c_template =
		NL "char testdata[] = \"abcdef\";"
		NL "const char testdata2[] = \"XY\";"
		NL "int try_pwrite(int fd)"
		NL "{"
		NL "	if (pwrite(fd, testdata2, 2, 3) != 2)"
		NL "		return 0;"
		NL "	testdata[3+0] = testdata2[0];"
		NL "	testdata[3+1] = testdata2[1];"
		NL "	return 1;"
		NL "}"
		NL "#include <stdio.h>"
		NL "#include <sys/types.h>"
		NL "#include <fcntl.h>"
		NL "int data_match(const char *b)"
		NL "{"
		NL "	return b[0]==testdata[0] && b[1]==testdata[1]"
		NL "	   &&  b[2]==testdata[2] && b[3]==testdata[3]"
		NL "	   &&  b[4]==testdata[4] && b[5]==testdata[5];"
		NL "}"
		NL "int main()"
		NL "{"
		NL "	char buf[6];"
		NL "	const char *filename = \"%s\";"
		NL "	int fd = open(filename, O_WRONLY);"
		NL no_implicit(ssize_t, "pwrite", "pwrite")
		NL "	write(fd, testdata, 6);"
		NL "	close(fd);"
		NL "	fd = open(filename, O_RDWR);"
		NL "	if (try_pwrite(fd) && read(fd, buf, 6) == 6 &&"
		NL "		data_match(buf))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	const char* incs[] = {"#include <unistd.h>","#define _XOPEN_SOURCE 600\n#include <unistd.h>",NULL};
	const char** inc;
	char test_c[8250];

	require("cc/cc", logdepth, fatal);

	report("Checking for pwrite()... ");
	logprintf(logdepth, "find_posix_pwrite: trying to find pwrite()...\n");
	logdepth++;

	tmpf = tempfile_new(".psx");
	sprintf(test_c, test_c_template, tmpf);

	for (inc = incs; *inc; ++inc) {
		if (try_icl(logdepth, node, test_c, *inc, NULL, NULL)) {
			unlink(tmpf);
			free(tmpf);
			return 0;
		}
	}
	unlink(tmpf);
	free(tmpf);
	return try_fail(logdepth, node);
}

int find_posix_glob(const char *name, int logdepth, int fatal)
{
	const char *key = "libs/posix/fs/glob";
	const char *test_c =
		NL "void my_puts(const char *s);"
		NL "int main()"
		NL "{"
		NL "	glob_t g;"
		NL "	if (glob(\"*\", 0, (void*)0, &g) == 0) {"
		NL "		globfree(&g);"
		NL "		my_puts(\"OK\");"
		NL "	}"
		NL "	return 0;"
		NL "}"
		NL "#include <stdio.h>"
		NL "void my_puts(const char *s)"
		NL "{"
		NL "	puts(s);"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for glob()... ");
	logprintf(logdepth, "find_posix_glob: trying to find glob()...\n");
	logdepth++;

	if (try_icl(logdepth, key, test_c, "#include <glob.h>", NULL, NULL))
		return 0;

	return try_fail(logdepth, key);
}

int find_posix_fnmatch(const char *name, int logdepth, int fatal)
{
	const char *key = "libs/posix/fs/fnmatch";
	const char *test_c =
		NL "void my_puts(const char *s);"
		NL "int main()"
		NL "{"
		NL "	if (fnmatch(\"*\", \"abc\", 0) == 0)"
		NL "		my_puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL "#include <stdio.h>"
		NL "void my_puts(const char *s)"
		NL "{"
		NL "	puts(s);"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for fnmatch()... ");
	logprintf(logdepth, "find_posix_fnmatch: trying to find fnmatch()...\n");
	logdepth++;

	if (try_icl(logdepth, key, test_c, "#include <fnmatch.h>", NULL, NULL))
		return 0;

	return try_fail(logdepth, key);
}
