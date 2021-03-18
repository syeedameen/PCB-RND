/*
    fungw - function gateway
    Copyright (C) 2017,2018,2019 Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/* These functions can be called from an scconfig hooks.c to get libfungw
   configured without having to have or compile or run a separate scconfig
   process for the lib. */

#include <stdio.h>
#include <string.h>
#include "arg.h"
#include "log.h"
#include "dep.h"
#include "db.h"
#include "libs.h"
#include "tmpasm_scconfig.h"

const char *fungw_ver1 = "1";
const char *fungw_ver2 = "1";
const char *fungw_ver3 = "0";

static void fungw_set_debug(const char *val)
{
	put("/local/fungw/debug", val);
}

static void fungw_set_prefix(const char *val)
{
	put("/local/fungw/prefix", val);
}

static void fungw_set_libdirname(const char *val)
{
	put("/local/fungw/libdirname", val);
}

static void fungw_set_pupdirname(const char *val)
{
	put("/local/fungw/pupdirname", val);
}

static int fungw_hook_postinit(void)
{
	db_mkdir("/local");
	put("/local/fungw/debug", sfalse);
	put("/local/fungw/ver1", fungw_ver1);
	put("/local/fungw/ver2", fungw_ver2);
	put("/local/fungw/ver3", fungw_ver3);
	put("/local/fungw/prefix", "/usr/local");
	put("/local/fungw/libdirname", "lib");
	put("/local/fungw/pupdirname", "lib/puplug");
	return 0;
}

static int fungw_hook_detect_host(void)
{
	if (istrue(get("/local/fungw/debug"))) {
		require("cc/argstd/Wall", 0, 0);
		require("cc/argstd/ansi", 0, 0);
	}

	require("fstools/ar", 0, 0);
	require("fstools/ranlib", 0, 0);

	return 0;
}

static int fungw_hook_detect_target(void)
{
	require("libs/ldl", 0, 0);
	require("cc/rdynamic", 0, 0);
	require("cc/soname", 0, 0);
	require("cc/so_undefined", 0, 0);
	require("cc/fpic", 0, 0);
	require("cc/wlrpath", 0, 0);
	require("cc/ldflags_dynlib", 0, 0);
	
	return 0;
}

/* Root is the path for the fungwlug lib dir (trunk/libfungw) */
static int fungw_hook_generate(const char *root)
{
	int generr = 0;

	fprintf(stderr, "Generating fungw/Makefile (%d)\n", generr |= tmpasm(root, "Makefile.in", "Makefile"));

	return generr;
}


static void fungw_print_configure_summary(void)
{
	const char *sum_in;
	fprintf(stderr, "\n\n== fungw configuration summary ==\n");

	fprintf(stderr, "  version: %s.%s.%s\n", fungw_ver1, fungw_ver2, fungw_ver3);

	sum_in = get("/local/fungw/summary");
	if ((sum_in != NULL) && (*sum_in != '\0')) {
		char *sep, *curr, *next, *sum, *state;
		while(*sum_in == ' ') sum_in++;
		sum = strclone(sum_in);
		for(curr = sum; (curr != NULL) && (*curr != '\0'); curr = next) {
			next = strchr(curr, ' ');
			if (next != NULL) {
				*next = '\0';
				next++;
				while(*next == ' ') next++;
			}
			sep = strchr(curr, ':');
			if (sep != NULL) {
				*sep = '\0';
				sep++;
				if (*sep == ':')
					sep++;
			}
			else
				sep = "u";
			switch(*sep) {
				case 'e': state = "enabled"; break;
				case 'd': state = "disabled"; break;
				default:  state = "unknown"; break;
			}
			fprintf(stderr, "  binding: %-20s %s\n", curr, state);
		}
		free(sum);
	}
	printf("\n\n");
}
