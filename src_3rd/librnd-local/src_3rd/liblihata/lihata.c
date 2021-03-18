/*
    liblihata - list/hash/table format, parser lib
    Copyright (C) 2013  Tibor 'Igor2' Palinkas

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Project URLs: http://repo.hu/projects/lihata
                  svn://repo.hu/lihata


   This file provides generic API for the most basic operations common to
   the event parser, the DOM parser, the tree functions and most probably
   even to the application code.
*/
#include <stdlib.h>
#include <string.h>
#include "lihata.h"

#define str_arr_size(a) (sizeof(a) / sizeof(const char *))

static const char *lht_type_names[] = {
	"invalid_type",
	"text",
	"list",
	"hash",
	"table",
	"symlink"
};
static const char *lht_type_ids[] = {
	"??",
	"te",
	"li",
	"ha",
	"ta",
	"sy"
};
#define lht_num_types str_arr_size(lht_type_names)

const char *lht_type_name(lht_node_type_t type_)
{
	int type = (int)type_; /* it's not guaranteed that type is signed */
	if ((type < 0) || (type >= lht_num_types))
		return NULL;
	return lht_type_names[type];
}

const char *lht_type_id(lht_node_type_t type_)
{
	int type = (int)type_; /* it's not guaranteed that type is signed */
	if ((type < 0) || (type >= lht_num_types))
		return NULL;
	return lht_type_ids[type];
}

lht_node_type_t lht_type_name2enum(const char *name)
{
	const char **s;
	lht_node_type_t t;
	for(s = lht_type_names+1, t = LHT_TEXT; *s != NULL; s++,t++)
		if (strcmp(*s, name) == 0)
			return t;
	return LHT_INVALID_TYPE;
}

lht_node_type_t lht_type_id2enum(const char *name)
{
	const char **s;
	lht_node_type_t t;
	for(s = lht_type_ids+1, t = LHT_TEXT; *s != NULL; s++,t++)
		if ((*s[0] == name[0]) && (*s[1] == name[1]))
			return t;
	return LHT_INVALID_TYPE;
}


static const char *lht_err_names[] = {
	"success: parser has finished, stop sending characters",
	"success",
	"internal error: parse state stack underflow",
	"internal error: invalid state after BSTRING is closed",
	"internal error: TVALUE finished in non-textual context",
	"invalid character (\0) in the stream",
	"unexpected EOF after backslash",
	"unexpected EOF in the middle of a brace string",
	"unexpected EOF in the middle of an open node",
	"unexpected EOF while waiting for a value",
	"unexpected EOF while waiting for or processing a text value",
	"invalid node type or unprotected colon as the 3rd character of a name",
	"unexpected colon in id (may require protection)",
	"can not escape before non-textual value",
	"invalid character between name and value",
	"invalid/unprotected character in text content",
	"li, ha or ta with no body",
	"internal parser error",
	"table row is not a list",
	"table rows with different number of columns",
	"duplicate key in hash",
	"node is not part of any document",
	"broken document",
	"path not found",
	"path: can't resolve relative path without a CWD",
	"path not found: child nodes don't have children",
	"path error: invalid integer in list or table addressing",
	"path: invalid node on path",
	"path: symlink recursion too deep",
	"path: underspecified table address: should be row/col, not row only",
	"node not found",
	"boundary check error: out of bounds",
	"merge failed: type mismatch",
	"merge failed: symlink mismatch",
	"merge failed: empty symlink",
	"merge failed: table column mismatch",
	"merge failed: cyclic merge request",
	"out of memory",
	"operation would render the document invalid",
	"node is not detached from its document"
};
#define lht_num_perrs str_arr_size(lht_err_names)

const char *lht_err_str(lht_err_t pe_)
{
	int pe = (int)pe_; /* ANSI C allows enum to be signed or unsigned */
	pe++;
	if ((pe < 0) || (pe > lht_num_perrs))
		return "invalid/unknown error";
	return lht_err_names[pe];
}

char *lht_strdup(const char *s)
{
	size_t len;
	char *n;

	if (s == NULL)
		return NULL;

	len = strlen(s);
	n = malloc(len+1);
	memcpy(n, s, len+1);
	return n;
}
