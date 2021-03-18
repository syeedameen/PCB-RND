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

int find_sul_dbus(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "#include <string.h>"
		NL "#include <dbus/dbus.h>"
		NL "int main()"
		NL "{"
		NL "	DBusError err;"
		NL "	dbus_error_init(&err);"
		NL "	puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	const char *node = "libs/sul/dbus";

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for dbus... ");

	if (try_icl_pkg_config(logdepth, "libs/sul/dbus", test_c, NULL, "dbus-*", get("/arg/dbus-version")))
		return 0;

	if (try_icl(logdepth, node, test_c, NULL, "-Idbus-1", "-ldbus"))
		return 0;

	return try_fail(logdepth, node);
}
