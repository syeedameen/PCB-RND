/*
    puplug - portable micro plugin framework
    Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas

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

/* These functions can be called from an scconfig hooks.c to get puplug
   configured without having to have or compile or run a separate scconfig
   process for the lib. */

#include <stdio.h>
#include <string.h>
#include "arg.h"
#include "log.h"
#include "dep.h"
#include "db.h"
#include "tmpasm_scconfig.h"

const char *pup_ver1 = "1";
const char *pup_ver2 = "0";
const char *pup_ver3 = "0";

static void pup_set_debug(const char *val)
{
		put("/local/pup/debug", val);
}

static void pup_hook_postinit(void)
{
	db_mkdir("/local");
	put("/local/pup/debug", sfalse);
	put("/local/pup/ver1", pup_ver1);
	put("/local/pup/ver2", pup_ver2);
	put("/local/pup/ver3", pup_ver3);
	put("/local/pup/disable_dynlib", sfalse);
}

static int pup_hook_detect_host(void)
{
	if (istrue(get("/local/pup/debug")))
		require("cc/argstd/Wall", 0, 0);

	require("fstools/ar", 0, 0);
	require("fstools/ranlib", 0, 0);

	return 0;
}

static int pup_hook_detect_target(void)
{
	require("libs/snprintf", 0, 0);
	require("libs/fs/readdir/*", 0, 0);
	require("libs/fs/findnextfile/*", 0, 0);
	require("cc/fpic", 0, 0);

	if (isfalse(get("/local/pup/disable_dynlib"))) {
		require("cc/wlrpath", 0, 0);
		require("libs/ldl", 0, 0);
		require("cc/rdynamic", 0, 0);
		require("cc/soname", 0, 0);
		require("cc/ldflags_dynlib", 0, 0);
	}

	return 0;
}

/* Root is the path for the puplug lib dir (trunk/puplug) */
static int pup_hook_generate(const char *root)
{
	int generr = 0;

	fprintf(stderr, "Generating puplug/Makefile (%d)\n", generr |= tmpasm(root, "Makefile.in", "Makefile"));
	fprintf(stderr, "Generating puplug/config.h (%d)\n", generr |= tmpasm(root, "config.h.in", "config.h"));

	return generr;
}
