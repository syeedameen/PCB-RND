/*
    scconfig - math feature detection
    Copyright (C) 2009  Tibor Palinkas

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
#include "find_func.h"
#include "find_mfunc_cc.h"
#include "find_fpenan.h"

int find_math_minpack(const char *name, int logdepth, int fatal)
{
	char *test_c =
		NL "int main() {"
		NL "	int one=1;"
		NL "	if (dpmpar_(&one) != 0.0)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for minpack... ");
	logprintf(logdepth, "find_math_minpack: trying to find minpack...\n");
	logdepth++;

	/* Look at the standard places */
	if (try_icl(logdepth, "libs/math/minpack", test_c, "#include <minpack.h>", NULL, "-lminpack")) return 0;

	return try_fail(logdepth, "libs/math/minpack");
}


void deps_math_init()
{
	dep_add("libs/math/minpack/*",  find_math_minpack);
	dep_add("libs/math/cc/log/*",   find_math_cc_log);
	dep_add("libs/math/isnan/*",    find_math_isnan);
	dep_add("libs/math/isinf/*",    find_math_isinf);
	dep_add("libs/math/isfinite/*", find_math_isfinite);
	dep_add("libs/math/isnormal/*", find_math_isnormal);
	dep_add("libs/math/nan/*",      find_math_nan);
	dep_add("libs/math/nanop/*",    find_math_nanop);

	dep_add("libs/math/expf/*",     find_math_expf);
	dep_add("libs/math/logf/*",     find_math_logf);
	dep_add("libs/math/rint/*",     find_math_rint);
	dep_add("libs/math/round/*",    find_math_round);
}
