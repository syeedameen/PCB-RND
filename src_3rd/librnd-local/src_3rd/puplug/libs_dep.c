/*
    puplug - portable micro plugin framework
    Copyright (C) 2017 Tibor 'Igor2' Palinkas

    libgpmi - General Package/Module Interface
    Copyright (C) 2005-2007 Patric 'TrueLight' Stout & Tibor 'Igor2' Palinkas

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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libs.h"
#include "error.h"


typedef struct {
	char *s, buff[PUP_PATH_MAX+64];
	int deps_alloced, conflict_alloced;
	int lineno;

	pup_context_t *pup;
	pup_plugin_t *lib;
	const char **dir_list;
	const char *pup_path;
} parse_dep_t;

#include "libs_dep_parse.h"


static void pup_conflict_announce(pup_context_t *pup, const char *who, const char *with_whom)
{
	char *tmp;
	tmp = malloc(strlen(who) + strlen(with_whom) + 40);
	sprintf(tmp, "package %s conflicts package %s", who, with_whom);
	pup_err_stack_push(pup, pup_err_conflict, pup_err_src_load_dep, tmp);
	free(tmp);
}

/* check if any of the existing plugins would conflict with lib */
int pup_conflict_loaded(pup_context_t *pup, const char *new_name)
{
	pup_plugin_t *p;
	for(p = pup->plugins; p != NULL; p = p->next) {
		int n;
		for(n = 0; n < p->conflict_len; n++)
			if (strcmp(p->conflict[n], new_name) == 0) {
				pup_conflict_announce(pup, p->name, new_name);
				return 1;
			}
	}
	return 0;
}

static int pup_conflict_loading(pup_context_t *pup, const char *oldn, const char *newn)
{
	if (pup_lookup(pup, oldn) != NULL) {
		pup_conflict_announce(pup, newn, oldn);
		return 1;
	}
	return 0;
}


static int pup_parse_dep(parse_dep_t *st, char *first, char *args)
{
	int veri = 0, state;
	pup_plugin_t *pkg;

	if (st->lib->flags & PUP_FLG_FIND_AUTO)
		return 0;

	if (args != NULL) {
		/* Atoi will stop at the next ';' - later we should use strtol here
		   if we are interested in flags after the 2nd separator */
		veri = atoi(args);
	}

	/* Try to load the package */
	pkg = pup_load_(st->pup, st->dir_list, NULL, first, veri, &state);

	/* Don't go in endless recursions becuase cyclic deps */
	if ((state == pup_err_already_loading) || (state == pup_err_cyclic_dep))
		return pup_err_stack_push(st->pup, pup_err_cyclic_dep, pup_err_src_load_dep, st->buff);

	/* Check for fatal errors */
	if (state < 0)
		return state;

	/* Append pkg to lib's dep list */
	if (st->lib->deps_len >= st->deps_alloced) {
		st->deps_alloced += 16;
		st->lib->deps = realloc(st->lib->deps, sizeof(pup_plugin_t *) * st->deps_alloced);
	}
	st->lib->deps[st->lib->deps_len] = pkg;
	st->lib->deps_len++;
	return 0;
}

static int pup_parse_conflict(parse_dep_t *st, char *first, char *args)
{
	if (st->lib->flags & PUP_FLG_FIND_AUTO)
		return 0;

	if (pup_conflict_loading(st->pup, first, st->lib->name))
		return 1;

	if (st->lib->conflict_len >= st->conflict_alloced) {
		st->conflict_alloced += 8;
		st->lib->conflict = realloc(st->lib->conflict, sizeof(char *) * st->conflict_alloced);
	}
	st->lib->conflict[st->lib->conflict_len] = pup_strdup(first);
	st->lib->conflict_len++;

	return 0;
}

static int pup_parse_autoload(parse_dep_t *st, char *first, char *args)
{
	st->lib->flags |= PUP_FLG_AUTOLOAD;
	return 0;
}

static int pup_parse_default(parse_dep_t *st, char *first, char *args) { return 0; }
static int pup_parse_tag(parse_dep_t *st, char *tag, char *args) { return 0; }


static int pup_parse_syntax_error(parse_dep_t *st, const char *msg)
{
	char tmp[PUP_PATH_MAX*2];
	sprintf(tmp, "syntax error in %s: %s\n", (st->pup_path == NULL ? "<buildin>" :st->pup_path), msg);
	pup_err_stack_push(st->pup, pup_err_parse_pup, pup_err_src_load_pup, tmp);
	return -1;
}

int pup_load_pup_file(pup_context_t *pup, const char **dir_list, pup_plugin_t *lib, const char *pup_file_name)
{
	FILE *f;
	int c, res = 0;
	parse_dep_t st;

	/* try to open the dep file of the library */
	f = fopen(pup_file_name, "r");
	if (f == NULL)
		return 0;

	memset(&st, 0, sizeof(st));
	pup_parse_pup_init(&st);
	st.pup = pup;
	st.lib = lib;
	st.dir_list = dir_list;
	st.pup_path = pup_file_name;

	/* Read the file char by char seeking words */
	do {
		c = fgetc(f);
		res = pup_parse_pup(&st, c);
	} while((res == 0) && (c != EOF));

	fclose(f);
	return res;
}

int pup_load_pup_str(pup_context_t *pup, const char **dir_list, pup_plugin_t *lib, const char *pup_script)
{
	int c, res = 0;
	parse_dep_t st;

	memset(&st, 0, sizeof(st));
	pup_parse_pup_init(&st);
	st.pup = pup;
	st.lib = lib;
	st.dir_list = dir_list;

	/* Read the file char by char seeking words */
	do {
		c = *pup_script;
		pup_script++;
		if (c == '\0')
			c = EOF;
		res = pup_parse_pup(&st, c);
	} while((res == 0) && (c != EOF));

	return res;
}

/**
 *  Unload dependency of a library recursively.
 */
void pup_unload_dep(pup_context_t *pup, pup_plugin_t *lib)
{
	int n;

	for(n = 0; n < lib->deps_len; n++)
		pup_unload(pup, lib->deps[n], NULL);

	free(lib->deps);
	lib->deps = NULL;

	for(n = 0; n < lib->conflict_len; n++)
		free(lib->conflict[n]);

	free(lib->conflict);
	lib->conflict_len = 0;
}

