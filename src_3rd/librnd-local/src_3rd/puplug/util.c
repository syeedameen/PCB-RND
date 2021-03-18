/*
    puplug - portable micro plugin framework
    Copyright (C) 2020 Tibor 'Igor2' Palinkas

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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "os_dep.h"
#include "os_dep_fs.h"
#include "util.h"


void pup_list_parse_pups(pup_list_parse_pup_t *ctx, const char **paths)
{
	const char **path;

	for(path = paths; *path != NULL; path++) {
		char fn[PUP_PATH_MAX*2], *fn_end;
		int dirlen;
		const char *fname;
		void *d = pup_open_dir(*path);

		if (d == NULL)
			continue;

		dirlen = strlen(*path);
		memcpy(fn, *path, dirlen);
		fn_end = fn + dirlen;
		*fn_end = '/';
		fn_end++;

		while((fname = pup_read_dir(d)) != NULL) {
			FILE *f;
			int len = strlen(fname);
			char *s, *e, line[1024];
			const char *end;

			if (len < 5)
				continue;

			end = fname + len -4;
			if (strcmp(end, ".pup") != 0)
				continue;

			strcpy(fn_end, fname);

			if ((ctx->open != NULL) && (ctx->open(ctx, fn, fname) != 0))
				continue;

			if ((ctx->line_raw != NULL) || (ctx->line_split != NULL)) {
				f = fopen(fn, "r");
				if (f == NULL)
					continue;
				while((s = fgets(line, sizeof(line), f)) != NULL) {
					int len;

					while(isspace(*s)) s++;
					len = strlen(s);
					if (len > 0) {
						e = s+len-1;
						while((e >= s) && isspace(*e)) { *e = '\0'; e--; }
					}

					if ((ctx->line_raw != NULL) && (ctx->line_raw(ctx, fn, s) != 0))
						break;
					if (ctx->line_split != NULL) {
						char *arg = strpbrk(s, " \t");
						if (arg != NULL) {
							*arg = '\0';
							arg++;
							while(isspace(*arg))
								arg++;
						}
						if (ctx->line_split(ctx, fn, s, arg) != 0)
							break;
					}
				}
				fclose(f);
			}

			if (ctx->close != NULL)
				ctx->close(ctx, fn);
		}
		pup_close_dir(d);
	}
}

