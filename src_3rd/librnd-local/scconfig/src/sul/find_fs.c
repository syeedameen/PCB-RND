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

int find_sul_libext2fs(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "#include <ext2fs/ext2fs.h>"
		NL "int main()"
		NL "{"
		NL "	ext2fs_inode_bitmap bm;"
		NL "	errcode_t ec;"
		NL "	ec = ext2fs_allocate_generic_bitmap(0, 32, 32, \"foo\", &bm);"
		NL "	if (ec == 0)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	const char *node = "libs/sul/libext2fs";
	const char **inc;
	const char *incs[] = {
		"#include <ext2fs/ext2_fs.h>",                     /* modern systems - real lib */
		"#undef umode_t\n#include <linux/ext2_fs.h>\n",    /* late '90s, e.g. Debian potato: header coming from the kernel */
		NULL};

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for ext2fs... ");

	for(inc = incs; *inc != NULL; inc++)
		if (try_icl(logdepth, node, test_c, *inc, NULL, "-lext2fs"))
			return 0;

	return try_fail(logdepth, node);
}

int find_sul_fuse(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "static struct fuse_operations oper;"
		NL "int main()"
		NL "{"
		NL "	char *argv[] = {\"fusetest\", \"-h\", NULL};"
		NL "	int ret;"
		NL "	fclose(stderr);"
		NL "	ret = fuse_main(2, argv, &oper, NULL);"
		NL "	if ((ret == 1) || (ret == 0))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	const char *node = "libs/sul/fuse";
	const char **inc;
	const char *incs[] = {
		"#define FUSE_USE_VERSION 26\n#include <fuse.h>",
		"#define _FILE_OFFSET_BITS 64\n#define FUSE_USE_VERSION 26\n#include <fuse.h>",
		"#if ((!defined _POSIX_C_SOURCE || (_POSIX_C_SOURCE - 0) < 199309L))\n#define _POSIX_C_SOURCE 199309\n#define FUSE_USE_VERSION 26\n#include <fuse.h>",
		"#if ((!defined _POSIX_C_SOURCE || (_POSIX_C_SOURCE - 0) < 199309L))\n#define _POSIX_C_SOURCE 199309\n#endif\n#define _FILE_OFFSET_BITS 64\n#define FUSE_USE_VERSION 26\n#include <fuse.h>",
		NULL};

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for fuse... ");

	for(inc = incs; *inc != NULL; inc++)
		if (try_icl(logdepth, node, test_c, *inc, NULL, "-lfuse"))
			return 0;

	return try_fail(logdepth, node);
}
