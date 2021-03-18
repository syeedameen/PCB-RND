/*
    scconfig - sul - software utility libraries
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
#include "libs.h"
#include "log.h"
#include "db.h"
#include "dep.h"

int find_sul_iconv(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "#include <stdlib.h>"
		NL "#include <iconv.h>"
		NL "int main()"
		NL "{"
		NL "	char   *inptr;"
		NL "	char   *outptr;"
		NL "	char    inbuf[20] = \"ABCDEFGH!@#$1234\";"
		NL "	char    outbuf[30];"
		NL "	size_t  inleft;"
		NL "	size_t  outleft;"
		NL "	iconv_t cd;"
		NL "	int     rc;"
		NL ""
		NL "	if ((cd = iconv_open(\"utf-8\", \"ascii\")) == (iconv_t)(-1))"
		NL "		return 1;"
		NL ""
		NL "	inleft  = 16;"
		NL "	outleft = 20;"
		NL "	inptr   = inbuf;"
		NL "	outptr  = outbuf;"
		NL ""
		NL "	rc = iconv(cd, &inptr, &inleft, &outptr, &outleft);"
		NL "	iconv_close(cd);"
		NL ""
		NL "	if (rc == (-1))"
		NL "		return 1;"
		NL ""
		NL "	puts(\"OK\");"
		NL ""
		NL "	return 0;"
		NL "}"
		NL;

	const char *node = "libs/sul/iconv";

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for iconv... ");

	if (try_icl(logdepth, node, test_c, NULL, NULL, NULL))
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, NULL, "-liconv"))
		return 0;

	return try_fail(logdepth, node);
}
