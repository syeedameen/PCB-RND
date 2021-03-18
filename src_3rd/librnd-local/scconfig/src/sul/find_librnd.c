/*
    scconfig - sul - software utility libraries
    Copyright (C) 2020  Tibor Palinkas

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

#define NODE3RD "libs/sul/librnd-3rd"

static int find_sul_librnd_3rd_(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "#include <genvector/vtd0.h>"
		NL "int main()"
		NL "{"
		NL "	vtd0_t v;"
		NL "	vtd0_init(&v);"
		NL "	if (vtd0_len(&v) == 0)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"

		NL;

	const char *node = NODE3RD;

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for librnd-3rd... ");

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/include/librnd/core -I/usr/include/librnd/src_3rd", "-lrnd-3rd"))
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/include/librnd/core -I/usr/include/librnd/src_3rd", "-lrnd-3rd"))
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/local/include -I/usr/local/include/librnd/core -I/usr/local/include/librnd/src_3rd", "-L/usr/local/lib -lrnd-3rd"))
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-I/usr/local/include -I/usr/local/include/librnd/core -I/usr/local/include/librnd/src_3rd", "-L/usr/local/lib -lrnd-3rd"))
		return 0;

	return try_fail(logdepth, node);
}

int find_sul_librnd_3rd(const char *name, int logdepth, int fatal)
{
	int res = find_sul_librnd_3rd_(name, logdepth, fatal);
	if (res == 0) {
		char *mak, *tmp, *end;

		/* hack: extract -L and edit it for the .mak path */
		tmp = strclone(get(NODE3RD "/ldflags"));
		end = strstr(tmp, "/lib");
		if (end != NULL)
			*end = '\0';
		put(NODE3RD"/root", tmp+2);
		mak = str_concat("", tmp+2, "/share/librnd/librnd.mak", NULL);
		put(NODE3RD"/mak", mak);
		free(tmp);
		free(mak);
	}
	return res;
}


int find_sul_librnd_hid(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "#include <vtc0.h>"
		NL "int main()"
		NL "{"
		NL "	pcb_coord_t coord;"
		NL "	vtc0_t v;"
		NL "	vtc0_init(&v);"
		NL "	if (vtc0_len(&v) == 0)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	const char *node = "libs/sul/librnd-hid";
	char *ldflags, *tmp, *end;

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require(NODE3RD, logdepth, fatal))
		return 1;

	report("Checking for librnd-hid... ");

	tmp = strclone(get(NODE3RD "/ldflags"));
	end = strstr(tmp, " -l");
	if (end != NULL)
		*end = '\0';
	ldflags = str_concat(" ", tmp, "-lrnd-hid -lrnd-3rd", NULL);
	free(tmp);

/* Can not run the test yet, because of genht */
#if 0
	if (try_icl(logdepth, node, test_c, NULL, get(NODE3RD "/cflags"), ldflags)) {
		free(ldflags);
		return 0;
	}
#else
	put("libs/sul/librnd-hid/presents", strue);
	put("libs/sul/librnd-hid/includes", "");
	put("libs/sul/librnd-hid/cflags", get(NODE3RD "/cflags"));
	put("libs/sul/librnd-hid/ldflags", ldflags);
	free(ldflags);
	return 0;
#endif


	free(ldflags);
	return try_fail(logdepth, node);
}
