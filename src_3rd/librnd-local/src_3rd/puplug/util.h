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

#ifndef PUPLUG_UTIL_H
#define PUPLUG_UTIL_H

typedef struct pup_list_parse_pup_s pup_list_parse_pup_t;

struct pup_list_parse_pup_s {

	/* user functions to do the actual processing; any of these may be NULL */
	int (*open)(pup_list_parse_pup_t *ctx, const char *path, const char *basename);  /* called before opening a .pup file (path is full path, basename is the file name only). Return non-zero to skip file */
	void (*close)(pup_list_parse_pup_t *ctx, const char *path); /* called after closing a .pup file (path) */
	int (*line_raw)(pup_list_parse_pup_t *ctx, const char *path, char *line); /* called for each line loaded from the file, leading and trailing whitespace removed, before parsing the line */
	int (*line_split)(pup_list_parse_pup_t *ctx, const char *path, char *cmd, char *args); /* called for each line loaded from the file; first field is passed in cmd, rest in args */

	/* fields used by the caller */
	void *user_data;
};

/* List all .pup files in all directories listed in paths, parse them and
   call ctx callbacks */
void pup_list_parse_pups(pup_list_parse_pup_t *ctx, const char **paths);

#endif
