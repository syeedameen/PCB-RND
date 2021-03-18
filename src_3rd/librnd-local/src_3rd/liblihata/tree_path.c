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


   This file contains path helpers (tree descend).
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "liblihata/dom.h"
#include "liblihata/tree.h"

#define LHT_SYMLINK_MAX_RECURSION 64

/* return the value of a symlink; if it's the empty string, return "." instead */
#define SYMLINK_VAL(_sy_) (*(_sy_)->data.symlink.value == '\0' ? "." : (_sy_)->data.symlink.value)

#define path_err(_err_, code) \
	do { \
		if (_err_ != NULL) \
			*_err_ = (code); \
		return NULL; \
	} while(0) \

#define reset_err(_err_) \
	do { \
		if (_err_ != NULL) \
			*_err_ = LHTE_SUCCESS; \
	} while(0) \

/* find the first sep in str (like strchr), but ignore those seps
   that are backslash protected */
static char *find_sep(char *str, char sep)
{
	char *s;
	if (*str == sep)
		return str;
	if (*str == '\0')
		return NULL;
	for(s = str+1; *s != '\0'; s++) {
		if ((s[-1] != '\\') && (s[0] == sep))
			return s;
	}
	return NULL;
}

/*
 * Used to separate name:snum
 * Input: char* _str_ (which is modified to have ':' replace with '\0'
 * Output: const char* _first_, _second_, pointing to the places in _str_.
 * If there is no colon, _first_ is NULL.
 */
#define split_at_colon(_str_, _first_, _second_)	\
	do { \
		char *colon_pos; \
    colon_pos = find_sep(_str_, ':'); \
		if (colon_pos != NULL) { \
			*colon_pos = '\0'; \
			_second_ = colon_pos + 1;	\
			_first_ = _str_; \
			if (*_second_ == '\0') \
				_second_ = "0";	\
		}	\
		else { \
			_second_ = _str_; \
			_first_ = NULL;	\
		} \
	} while(0)

/* parse path segment "child" and return the child node of current
   that matches (or NULL if not found) */
static const lht_node_t *path_descend(const lht_node_t *current, char *child, lht_err_t *err, char **tbl, int follow_sy, int *depth)
{
	const char *name, *snum;
	char *end;
	int num;

	switch(current->type) {
		case LHT_INVALID_TYPE:
			path_err(err, LHTE_PATH_INVALID_NODE);
		case LHT_TEXT:
			path_err(err, LHTE_PATH_CHILD_OF_TEXT);
		case LHT_SYMLINK:
			/* resolve symlink into current */
			(*depth)++;
			current = lht_tree_path_(current->doc, current->parent, SYMLINK_VAL(current), follow_sy, *depth, err);
			if (current == NULL)
				return NULL;
			/* go on searching for child under the new node */
			return path_descend(current, child, err, tbl, follow_sy, depth);
		case LHT_LIST:
			split_at_colon(child, name, snum);
			num = strtol(snum, &end, 10);
			if (*end != '\0')
				path_err(err, LHTE_PATH_INT);
			if (name == NULL)
				return lht_tree_list_nth(current, num);
			return lht_tree_list_nthname(current, num, name);
		case LHT_HASH:
			return lht_dom_hash_get(current, child);
		case LHT_TABLE:
			/* tables are shifted in two phases because of the separator between row and col is the same as the path separator */
			if (*tbl == NULL) {
				*tbl = child;
				return current;
			}
			else {
				const char *rname, *srnum, *cname, *scnum;
				int rnum, cnum;

				split_at_colon(*tbl, rname, srnum);
				rnum = strtol(srnum, &end, 10);
				if (*end != '\0')
					path_err(err, LHTE_PATH_INT);

				split_at_colon(child, cname, scnum);
				cnum = strtol(scnum, &end, 10);
				if (*end != '\0')
					path_err(err, LHTE_PATH_INT);

				*tbl = NULL;
				return lht_tree_table_named_cell(current, rname, rnum, cname, cnum);
			}
	}
	path_err(err, LHTE_PATH_INVALID_NODE);
}

/* split up path and follow each segment - return the node it led to */
static const lht_node_t *path_follow(const lht_node_t *from, char *path, lht_err_t *err, int follow_sy, int *depth)
{
	char *next;
	char *tbl = NULL;

	do {
		next = find_sep(path, '/');
		if (next != NULL) {
			*next = '\0';
			next++;
		}
		if (*path != '\0') {
			if (strcmp(path, ".") == 0) {
				if (from->parent != NULL)
					from = from->parent;
			}
			if (strcmp(path, "..") == 0) {
				if (from->parent != NULL)
					from = from->parent;
				if (from->parent != NULL)
					from = from->parent;
			}
			else  {
				from = path_descend(from, path, err, &tbl, follow_sy, depth);
				if ((from == NULL) && (tbl == NULL)) /* child not found */
						path_err(err, LHTE_PATH_NOT_FOUND);
			}
			/* else do nothing: . is the current node in from */
		}
		path = next;
	} while(next != NULL);
	if (tbl != NULL)
		path_err(err, LHTE_PATH_UNDERSPEC_TABLE);
	return from;
}

lht_node_t *lht_tree_path_(lht_doc_t *doc, const lht_node_t *cwd, const char *path_, int follow_sy, int depth, lht_err_t *err)
{
	const lht_node_t *current;
	char *path = lht_strdup(path_);

	reset_err(err);

	if (*path == '/')
		current = path_follow(doc->root, path+1, err, follow_sy, &depth); /* absolute path */
	else
		current = path_follow(cwd, path, err, follow_sy, &depth);   /* relative path (to cwd) */

	free(path);

	/* follow symlink if required */
	if ((follow_sy) && (current != NULL) && (current->type == LHT_SYMLINK)) {
		if (depth >= LHT_SYMLINK_MAX_RECURSION)
				path_err(err, LHTE_PATH_SYMLINK_TOO_DEEP);
		return lht_tree_path_(doc, current->parent, SYMLINK_VAL(current), follow_sy, depth+1, err);
	}

	return (lht_node_t *)current;
}

lht_node_t *lht_tree_path(lht_doc_t *doc, const char *cwd_, const char *path, int follow_sy, lht_err_t *err)
{
	const lht_node_t *cwd_node;

	reset_err(err);

	if (*path != '/') {
		char *cwd;
		int depth = 0;

		if (cwd_ == NULL)
			path_err(err, LHTE_PATH_UNKNOWN_CWD);
		/* relative to cwd */
		if (*cwd_ == '/')
			cwd_++;
		cwd = lht_strdup(cwd_);

		cwd_node = path_follow(doc->root, cwd, err, follow_sy, &depth);
		free(cwd);
		if (cwd_node == NULL)
			path_err(err, LHTE_PATH_NOT_FOUND);
	}
	else
		cwd_node = NULL;

	return lht_tree_path_(doc, cwd_node, path, follow_sy, 0, err);
}

char *lht_tree_path_build(lht_node_t *cwd, lht_node_t *node, lht_err_t *err)
{
	lht_node_t *start, *tmp;
	int len = 0; /* no need to allocate for the terminating '\0': it will be in place of the last "/" */
	char *path, *path_start;
	char buff[64];
	int row, col;

	if ((cwd != NULL) && (cwd == node))
		return lht_strdup(".");

	if ((cwd == NULL) && (node->parent == NULL))
		return lht_strdup("/");

	start = cwd != NULL ? cwd : node->doc->root;

	for (tmp = node; tmp != NULL && tmp != start; tmp = tmp->parent) {
		switch (tmp->parent->type) {
			case LHT_TEXT:
			case LHT_HASH:
			case LHT_SYMLINK:
				len += strlen(tmp->name) + 1; /* this last is for '/' */
				break;
			case LHT_LIST:
				sprintf(buff, "%d", lht_tree_list_find_node(tmp->parent, tmp));
				len += strlen(buff) + 1;
				break;
			case LHT_TABLE:
				lht_tree_table_find_cell(tmp->parent, tmp, &row, &col);
				sprintf(buff, "%d/%d", row, col);
				len += strlen(buff) + 1;
				break;
			case LHT_INVALID_TYPE:
				if (err != NULL)
					*err = LHTE_BROKEN_DOC;
				return NULL;
				break;
		}
	}

	if (tmp == NULL)
		return NULL; /* not found as child of cwd */
	else if (tmp->parent == NULL)
		len++; /* final '\0' for absolute path */

	path_start = path = malloc(len);
	path += len - 1;
	*path = 0;
	for (tmp = node; (tmp->parent != NULL) && (tmp != cwd); tmp = tmp->parent) {
		switch (tmp->parent->type) {
			case LHT_TEXT:
			case LHT_HASH:
			case LHT_SYMLINK:
				len = strlen(tmp->name);
				path -= len;
				memcpy(path, tmp->name, len);
				break;
			case LHT_LIST:
				sprintf(buff, "%d", lht_tree_list_find_node(tmp->parent, tmp));
				len = strlen(buff);
				path -= len;
				memcpy(path, buff, len);
				break;
			case LHT_TABLE:
				lht_tree_table_find_cell(tmp->parent, tmp, &row, &col);
				sprintf(buff, "%d/%d", row, col);
				len = strlen(buff);
				path -= len;
				memcpy(path, buff, len);
				break;
			case LHT_INVALID_TYPE:
				if (err != NULL)
					*err = LHTE_BROKEN_DOC;
				return NULL;
				break;
		}
		if (tmp == start)
			break;
		path--;
		*path = '/';
	}

/* we must have reached the beginning of the string */
	assert(path == path_start);

	if (err != NULL)
		*err = LHTE_SUCCESS;

	return path_start;
}
