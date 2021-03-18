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
#include <ctype.h>
#include <string.h>

/* caller must define these: */
static int pup_parse_dep(parse_dep_t *st, char *first, char *args);
static int pup_parse_conflict(parse_dep_t *st, char *first, char *args);
static int pup_parse_autoload(parse_dep_t *st, char *first, char *args);
static int pup_parse_default(parse_dep_t *st, char *first, char *args);
static int pup_parse_tag(parse_dep_t *st, char *tag, char *args);
static int pup_parse_syntax_error(parse_dep_t *st, const char *msg);


static void pup_parse_pup_init(parse_dep_t *st)
{
	st->s = st->buff;
	*st->s = 0;
	st->lineno = 0;
}


static int pup_parse_pup(parse_dep_t *st, int c)
{
	char *first, *args, *cmd;
	int state;

	switch(c) {
		/* end of word */
		case '\n':
		case '\r':
			st->lineno++;
		case EOF:
		case ';':
		case '\0':
			*st->s = '\0';

			cmd = st->buff;
			while(isspace(*cmd)) cmd++;
			if ((*cmd != '\0') && (*cmd != '#')) {
				first = strpbrk(cmd, " \t");
				if (first != NULL) {
					*first = '\0';
					first++;
					while(isspace(*first)) first++;
				}
				/* Split version number by replacing the first ',' with '\0' */
				if (first != NULL) {
					args = strchr(first, ',');
					if (args != NULL) {
						*args = 0;
						args++;
					}
				}
				else
					args = NULL;

				if (strcmp(cmd, "dep") == 0) state = pup_parse_dep(st, first, args);
				else if (strcmp(cmd, "conflict") == 0) state = pup_parse_conflict(st, first, args);
				else if (strcmp(cmd, "autoload") == 0) state = pup_parse_autoload(st, first, args);
				else if (strcmp(cmd, "default") == 0) state = pup_parse_default(st, first, args);
				else if (*cmd == '$') {
					if (args != NULL)
						args[-1] = ',';
					state = pup_parse_tag(st, cmd+1, first);
				}
				else state = pup_parse_syntax_error(st, cmd);

				if (state < 0)
					return state;
			}

			st->s   = st->buff;
			*st->s  = 0;
			break;

		/* copy next character */
		default:
			if ((st->s - st->buff) >= sizeof(st->buff)) {
				char tmp[128];
				sprintf(tmp, "line %d too long: %ld >= %ld", st->lineno, (long int) (st->s - st->buff), (long int) sizeof(st->buff));
				pup_parse_syntax_error(st, tmp);
			}
			else {
				*st->s = c;
				st->s++;
			}
	}
	return 0;
}
