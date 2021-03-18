/*
    liblihata - list/hash/table format, tree tester tool
    Copyright (C) 2013, 2016  Tibor 'Igor2' Palinkas
    Copyright (C) 2013  Gabor Horvath (HvG)

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


   This file contains a minimalistic interpreter for varying our tests
   on the lht_tree* functionality.
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "dom.h"
#include "tree.h"

lht_doc_t *doc[32];
lht_node_t *node[32];
int lineno;

void load_file(lht_doc_t **doc, const char *fn)
{
	FILE *f;

	f = fopen(fn, "r");
	if (f == NULL) {
		fprintf(stderr, "can not open file '%s' for read\n", fn);
		exit(1);
	}
	*doc = lht_dom_init();
	while(!(feof(f))) {
		lht_err_t err;
		int c = fgetc(f);
		err = lht_dom_parser_char(*doc, c);
		if (err != LHTE_SUCCESS) {
			if (err != LHTE_STOP) {
				fprintf(stderr, "* error in '%s': %s *\n", fn, lht_err_str(err));
				exit(1);
			}
			break; /* error or stop, do not read anymore (would get LHTE_STOP without any processing all the time) */
		}
	}
	fclose(f);
}

static void shift_ws(char **args)
{
	while((**args != '\0') && isspace(**args)) (*args)++;
}

/* Shift next argument as string, up to the _first_ whitespace. This means
   if the first character is a whitespace, empty string is returned. The
   string returned shall be free'd by the caller. */
static char *shift_str(char **args)
{
	char *s, *start;
	int len;

	start = *args;
	while((**args != '\0') && (!isspace(**args))) (*args)++;
	len = *args - start;
	s = malloc(len+1);
	memcpy(s, start, len);
	s[len] = '\0';
	return s;
}

static int shift_int(char **args)
{
	char *end;
	int n;

	n = strtol(*args, &end, 10);
	if ((*end != '\0') && (*end != ' ') && (*end != '\t')) {
		fprintf(stderr, "expected an integer, got something else ('%s')\n", *args);
		exit(1);
	}

	*args = end;
	return n;
}

static int shift_id(char **args, char *type, char idchar, int arrsize)
{
	char *end;
	int id;

	shift_ws(args);
	if (**args == '\0') {
		fprintf(stderr, "expected a %s ID, got nothing\n", type);
		exit(1);
	}
	if (**args != idchar) {
		fprintf(stderr, "expected a %s ID, got something else ('%s'; ID problem)\n", type, *args);
		exit(1);
	}
	(*args)++;

	id = strtol(*args, &end, 10);
	if ((*end != '\0') && (*end != ' ') && (*end != '\t')) {
		fprintf(stderr, "expected a %s ID, got something else ('%s'; integer problem)\n", type, *args);
		exit(1);
	}

	if ((id < 0) || (id >= arrsize)) {
		fprintf(stderr, "document ID is out of range ('%s')\n", *args);
		exit(1);
	}

	*args = end;
	return id;
}

static lht_doc_t **shift_doc(char **args)
{
	return doc + shift_id(args, "document", 'D', sizeof(doc) / sizeof(lht_doc_t *));
}

static lht_node_t **shift_node(char **args)
{
	return node + shift_id(args, "node", 'N', sizeof(node) / sizeof(lht_node_t *));
}

void cmd_load_doc(char *args)
{
	lht_doc_t **doc;

	doc = shift_doc(&args);
	shift_ws(&args);
	load_file(doc, args);
}

void cmd_print_doc(char *args)
{
	lht_doc_t **doc;

	doc = shift_doc(&args);
	if (*doc == NULL) {
		printf("print_doc: doc was NULL\n");
		return;
	}
	if ((*doc)->root == NULL)
			printf("  * empty document *\n");
		else
			lht_dom_ptree((*doc)->root, stdout, "  ");
}

void cmd_print_node(char *args)
{
	lht_node_t **node;

	node = shift_node(&args);
	if ((*node) == NULL)
			printf("  * node is NULL *\n");
		else
			lht_dom_pnode(*node, stdout, "  ");
}

void cmd_new_text(char *args)
{
	lht_node_t **node;
	char *s, *sep;

	node = shift_node(&args);
	shift_ws(&args);
	s = shift_str(&args);
	sep = strchr(s, '=');
	if (sep != NULL) {
		*sep = '\0';
		*node = lht_dom_node_alloc(LHT_TEXT, s);
		(*node)->data.text.value = lht_strdup(sep+1);
		free(s);
	}
	else {
		*node = lht_dom_node_alloc(LHT_TEXT, "");
		(*node)->data.text.value = s;
	}
}

void cmd_print_tree(char *args)
{
	lht_node_t **node;

	node = shift_node(&args);
	if ((*node) == NULL)
			printf("  * node is NULL *\n");
		else
			lht_dom_ptree(*node, stdout, "  ");
}

void cmd_export(char *args)
{
	lht_node_t **node;
	lht_err_t err;

	node = shift_node(&args);
	if ((*node) == NULL) {
		printf("  * node is NULL *\n");
		return;
	}

	err = lht_dom_export(*node, stdout,"  ");
	if (err != LHTE_SUCCESS)
		printf("export: %s\n", lht_err_str(err));
}

void cmd_free_doc(char *args)
{
	lht_doc_t **doc;

	doc = shift_doc(&args);
	lht_dom_uninit(*doc);
}

void cmd_root(char *args)
{
	lht_node_t **node;
	lht_doc_t **doc;

	node = shift_node(&args);
	doc = shift_doc(&args);
	*node = (*doc)->root;
}

void cmd_list_next(char *args)
{
	lht_node_t **node;

	node = shift_node(&args);
	if ((*node)->parent == NULL) {
		fprintf(stderr, "node is not in a list (root node)\n");
		exit(1);
	}
	if ((*node)->parent->type != LHT_LIST) {
		fprintf(stderr, "node is not in a list\n");
		exit(1);
	}
	(*node) = (*node)->next;
}

void cmd_replace(char *args)
{
	lht_node_t **dst, **src;
	shift_ws(&args);
	dst = shift_node(&args);
	src = shift_node(&args);

	if ((*dst)->parent == NULL) {
		fprintf(stderr, "target node is not in a document (detached, floating)\n");
		exit(1);
	}
	if ((*src)->parent != NULL) {
		fprintf(stderr, "source node is not detached\n");
		exit(1);
	}


	lht_tree_replace(*dst, *src);
}

void cmd_list_child(char *args)
{
	lht_node_t **node;

	node = shift_node(&args);
	if ((*node)->type != LHT_LIST) {
		fprintf(stderr, "node is not a list\n");
		exit(1);
	}
	(*node) = (*node)->data.list.first;
}

void cmd_list_nth(char *args)
{
	lht_node_t **node;
	int n;

	node = shift_node(&args);
	shift_ws(&args);
	n = shift_int(&args);
	*node = lht_tree_list_nth(*node, n);
}

void cmd_list_del_nth(char *args)
{
	lht_node_t **node;
	int n;
	lht_err_t err;

	node = shift_node(&args);
	shift_ws(&args);
	n = shift_int(&args);

	err = lht_tree_list_del_nth(*node, n);
	if (err != LHTE_SUCCESS)
		printf("list_del_nth: %s\n", lht_err_str(err));
}

void cmd_list_del_child(char *args)
{
	lht_node_t **list, **child;

	list = shift_node(&args);
	child = shift_node(&args);
	printf("list_del_child: %d\n", lht_tree_list_del_child(*list, *child));
	*child = NULL;
}

void cmd_list_nthname(char *args)
{
	lht_node_t **node;
	int n;

	node = shift_node(&args);
	shift_ws(&args);
	n = shift_int(&args);
	shift_ws(&args);
	*node = lht_tree_list_nthname(*node, n, args);
}


void cmd_list_find_node(char *args)
{
	lht_node_t **list;
	lht_node_t **node;
	int res;

	list = shift_node(&args);
	assert(list != NULL);
	shift_ws(&args);
	node = shift_node(&args);
	assert(node != NULL);
	shift_ws(&args);

	res = lht_tree_list_find_node(*list, *node);
	printf("lht_tree_list_find_node: %d\n", res);
}


void cmd_table_cell(char *args)
{
	lht_node_t **node;
	int line, col;

	node = shift_node(&args);
	shift_ws(&args);
	line = shift_int(&args);
	shift_ws(&args);
	col = shift_int(&args);
	*node = lht_dom_table_cell(*node, line, col);
}

void cmd_tree_merge(char *args)
{
	lht_node_t **dst, **src;
	lht_err_t err;

	dst = shift_node(&args);
	src = shift_node(&args);
	err = lht_tree_merge(*dst, *src);
	if (err != LHTE_SUCCESS)
		printf("merge error: %s\n", lht_err_str(err));
	else
		*src = NULL;
}

#define table_op_int(name) \
void cmd_table_ ## name(char *args) \
{ \
	lht_node_t **node; \
	int i; \
	lht_err_t err; \
	node = shift_node(&args); \
	shift_ws(&args); \
	i = shift_int(&args); \
	err = lht_tree_table_ ## name(*node, i); \
	if (err == LHTE_SUCCESS) \
		return; \
	fprintf(stdout, "table_" #name ": %s\n", lht_err_str(err)); \
}

table_op_int(ins_col);
table_op_int(ins_row);
table_op_int(del_col);
table_op_int(del_row);

void cmd_path_(char *args, int follow_sy)
{
	lht_node_t **dst;
	lht_err_t err;

	dst = shift_node(&args);
	assert(dst != NULL);
	shift_ws(&args);

	*dst = lht_tree_path_((*dst)->doc, *dst, args, follow_sy, 0, &err);
	if (err != LHTE_SUCCESS)
		printf("Path error: %s\n", lht_err_str(err));
}

void cmd_path(char *args)
{
	cmd_path_(args, 1);
}



void cmd_path_build(char *args)
{
	lht_node_t **cwd;
	lht_node_t **node;
	char *res;
	lht_err_t err;

	cwd = shift_node(&args);
	assert(cwd != NULL);
	shift_ws(&args);
	node = shift_node(&args);
	assert(node != NULL);
	shift_ws(&args);

	res = lht_tree_path_build(*cwd, *node, &err);
	if (err != LHTE_SUCCESS)
		printf("lht_tree_path_build: broken document\n");
	else
		printf("lht_tree_path_build: %s\n", res);
	free(res);
}


void cmd_path_nofollow(char *args)
{
	cmd_path_(args, 0);
}

void cmd_equ_node(char *args)
{
	lht_node_t **dst, **src;

	dst = shift_node(&args);
	src = shift_node(&args);

	*dst = *src;
}

void cmd_equ_doc(char *args)
{
	lht_doc_t **dst, **src;

	dst = shift_doc(&args);
	src = shift_doc(&args);

	*dst = *src;
}

void cmd_echo(char *args)
{
	printf("%s\n", args);
}

void cmd_tree_has_symlink(char *args)
{
	lht_node_t **node;
	int result;
	char yes[] = "Tree has symlink(s).\n";
	char no[] = "Tree doesn't have symlink.\n";
	node = shift_node(&args);
	result = lht_tree_has_symlink(*node, 0);
	if (result)
		printf("%s", yes);
	else
		printf("%s", no);
}

void cmd_symlink_is_broken(char *args)
{
	lht_node_t **parent, **symlink;
	int result;
	char yes[] = "Symlink is broken.\n";
	char no[] = "Symlink is OK.\n";
	parent = shift_node(&args);
	symlink = shift_node(&args);
	result = lht_tree_symlink_is_broken(*parent, *symlink);
	if (result)
		printf("%s", yes);
	else
		printf("%s", no);
}

void cmd_table_find_cell(char *args)
{
	lht_node_t **table, **cell;
	int result, row, col;
	table = shift_node(&args);
	cell = shift_node(&args);
	result = lht_tree_table_find_cell(*table, *cell, &row, &col);
	if (result)
		printf("Cell was found in row %d, col %d.\n", row, col);
	else
		printf("%s", "Cell wasn't found in table.\n");
}

void cmd_table_detach_cell(char *args)
{
	lht_node_t **table;
	lht_node_t *result;
	lht_node_t **out;
	int row, col;
	lht_err_t err;
	table = shift_node(&args);
	row = shift_int(&args);
	col = shift_int(&args);
	out = shift_node(&args);
	result = lht_tree_table_detach_cell(*table, row, col, &err);
	if (err != LHTE_SUCCESS)
		printf("table_detach_cell: error: %s\n", lht_err_str(err));
	else
		printf("table_detach_cell: %d;%d success\n", row, col);
	*out = result;
}

void cmd_table_replace_cell(char *args)
{
	lht_node_t **table, **source, **out;
	lht_node_t *result;
	int row, col;
	lht_err_t err;
	table = shift_node(&args);
	row = shift_int(&args);
	col = shift_int(&args);
	source = shift_node(&args);
	out = shift_node(&args);

	result = lht_tree_table_replace_cell(*table, row, col, &err, *source);
	if (err != LHTE_SUCCESS)
		printf("table_replace_cell: error: %s\n", lht_err_str(err));
	else
		printf("table_replace_cell: %d/%d success\n", row, col);
	*out = result;
}

void cmd_table_detach_child(char *args)
{
	lht_node_t **table, **child;
	lht_err_t err;

	table = shift_node(&args);
	child = shift_node(&args);

	err = lht_tree_table_detach_child(*table, *child);

	if (err != LHTE_SUCCESS)
		printf("table_detach_child error: %s\n", lht_err_str(err));
}

void cmd_node_is_under(char *args)
{
	lht_node_t **source, **parent;
	int result;
	char *srcname = "", *prntname = "";

	source = shift_node(&args);
	parent = shift_node(&args);
	if ((*source)->name != NULL)
		srcname = (*source)->name;
	if ((*parent)->name != NULL)
		prntname = (*parent)->name;
	result = lht_tree_is_under(*source, *parent);
	if (result)
		printf("Node '%s' is under node '%s'\n", srcname, prntname);
	else
		printf("Node '%s' is NOT under node '%s'\n", srcname, prntname);
}

void cmd_list_replace_child(char *args)
{
	lht_node_t **list, **current, **new;
	lht_err_t result;

	list = shift_node(&args);
	current = shift_node(&args);
	new = shift_node(&args);
	result = lht_tree_list_replace_child(*list, *current, *new);
	if (result == LHTE_SUCCESS)
		printf("list_replace_child: done\n");
	else
		printf("Error during list_replace_child: %s\n", lht_err_str(result));
}

void cmd_list_detach_child(char *args)
{
	lht_node_t **list, **node;
	lht_err_t result;

	list = shift_node(&args);
	node = shift_node(&args);
	result = lht_tree_list_detach_child(*list, *node);
	if (result == LHTE_SUCCESS)
		printf("list_detach_child: done\n");
	else
		printf("Error during list_detach_child: %s\n", lht_err_str(result));
}

void cmd_table_replace_child(char *args)
{
	lht_node_t **table, **current, **new;
	lht_err_t result;

	table = shift_node(&args);
	current = shift_node(&args);
	new = shift_node(&args);
	result = lht_tree_table_replace_child(*table, *current, *new);
	if (result == LHTE_SUCCESS)
		printf("table_replace_child: done\n");
	else
		printf("Error during table_replace_child: %s\n", lht_err_str(result));
}

void cmd_tree_unlink(char *args)
{
	lht_node_t **node;
	lht_err_t result;

	node = shift_node(&args);
	result = lht_tree_unlink(*node);
	if (result == LHTE_SUCCESS)
		printf("tree_unlink: done\n");
	else
		printf("Error during tree_unlink: %s\n", lht_err_str(result));
}

void cmd_tree_detach(char *args)
{
	lht_node_t **node;
	lht_err_t result;

	node = shift_node(&args);
	result = lht_tree_detach(*node);
	if (result == LHTE_SUCCESS)
		printf("tree_detach: done\n");
	else
		printf("Error during tree_detach: %s\n", lht_err_str(result));
}

void cmd_list_detach_nth(char *args)
{
	lht_node_t **node;
	lht_node_t *result;
	int nth;

	node = shift_node(&args);
	nth = shift_int(&args);
	result = lht_tree_list_detach_nth(*node, nth);
	if (result != NULL)
		printf("list_detach_nth: done\n");
	else
		printf("list_detach_nth: NULL was returned\n");
}

void cmd_table_named_cell(char *args)
{
	lht_node_t **table, **dest;
	char *rowname, *colname;
	int row, col;

	table = shift_node(&args);
	shift_ws(&args);
	rowname = shift_str(&args);
	row = shift_int(&args);
	shift_ws(&args);
	colname = shift_str(&args);
	col = shift_int(&args);
	dest = shift_node(&args);
	*dest = lht_tree_table_named_cell(*table, rowname, row, colname, col);
	free(rowname);
	free(colname);
}

void cmd_dom_load(char *args)
{
	char *docname;
	lht_doc_t **d;
	char *errmsg;

	d = shift_doc(&args);
	shift_ws(&args);
	docname = shift_str(&args);

	*d = lht_dom_load(docname, &errmsg);
	if (*d == NULL) {
		printf("dom_load: %s", errmsg);
		free(errmsg);
	}
	free(docname);
}

void cmd_dom_iterate(char *args)
{
	lht_node_t **parent;
	lht_dom_iterator_t it;
	lht_node_t *child;

	parent = shift_node(&args);
	child = lht_dom_first(&it, *parent);

	if (child == NULL) {
		printf("  * node has no children *\n");
		return;
	}

	while (child != NULL) {
		lht_dom_pnode(child, stdout, "  ");
		child = lht_dom_next(&it);
	}
}

void cmd_dom_node_free(char *args)
{
	lht_node_t **node;

	node = shift_node(&args);
	lht_dom_node_free(*node);
}

void cmd_dom_list_append(char *args)
{
	lht_node_t **dest, **node;
	lht_err_t err;

	dest = shift_node(&args);
	node = shift_node(&args);

	err = lht_dom_list_append(*dest, *node);
	if (err != LHTE_SUCCESS)
		printf("Error during dom_list_append: %s\n", lht_err_str(err));
}

void cmd_dom_list_insert(char *args)
{
	lht_node_t **dest, **node;
	lht_err_t err;

	dest = shift_node(&args);
	node = shift_node(&args);

	err = lht_dom_list_insert(*dest, *node);
	if (err != LHTE_SUCCESS)
		printf("Error during dom_list_insert: %s\n", lht_err_str(err));
}

void cmd_dom_list_insert_after(char *args)
{
	lht_node_t **dest, **node;
	lht_err_t err;

	dest = shift_node(&args);
	node = shift_node(&args);

	err = lht_dom_list_insert_after(*dest, *node);
	if (err != LHTE_SUCCESS)
		printf("Error during dom_list_insert_after: %s\n", lht_err_str(err));
}

void cmd_dom_list_len(char *args)
{
	lht_node_t **node;
	int len;

	node = shift_node(&args);
	len = lht_dom_list_len(*node);
	printf("Len of the list: %d\n", len);
}

void cmd_dom_hash_put(char *args)
{
	lht_node_t **dest, **node;
	lht_err_t result;

	dest = shift_node(&args);
	node = shift_node(&args);

	result = lht_dom_hash_put(*dest, *node);

	if (result != LHTE_SUCCESS)
		printf("Error during dom_hash_put: %s\n", lht_err_str(result));
}

void cmd_dom_hash_get(char *args)
{
	lht_node_t **parent, **node;
	char *key;

	parent = shift_node(&args);
	node = shift_node(&args);
	shift_ws(&args);
	key = shift_str(&args);

	*node = lht_dom_hash_get(*parent, key);
	free(key);
}

void cmd_tree_del(char *args)
{
	lht_node_t **node;
	lht_err_t err;

	node = shift_node(&args);
	if (node == NULL) {
		printf("Error in tree_del: attempt to delete NULL\n");
		return;
	}

	err = lht_tree_del(*node);
	if (err != LHTE_SUCCESS)
		printf("Error in tree_del: %s\n", lht_err_str(err));
	else
		*node = NULL;
}

void cmd_tree_dup(char *args)
{
	lht_node_t **dst, **src;

	dst = shift_node(&args);
	if (dst == NULL) {
		printf("Error in tree_dup: attempt to dup into NULL\n");
		return;
	}

	src = shift_node(&args);
	if (src == NULL) {
		printf("Error in tree_dup: attempt to dup NULL\n");
		return;
	}

	*dst = lht_dom_duptree(*src);
	if (*dst == NULL)
		printf("Error in tree_dup\n");
}


typedef struct {
	const char *name;
	void (*cmd)(char *args);
} cmd_t;

cmd_t cmds[] = {
	{"load_doc", cmd_load_doc},                      /* Dtarget filename */
	{"free_doc", cmd_free_doc},                      /* Dtarget */
	{"print_doc", cmd_print_doc},                    /* Dsource */
	{"print_node", cmd_print_node},                  /* Nsource */
	{"print_tree", cmd_print_tree},                  /* Nsource */
	{"export", cmd_export},                          /* Nsource */
	{"replace", cmd_replace},                        /* Ndest Nsource */
	{"new_text", cmd_new_text},                      /* Ntarget text */
	{"list_next", cmd_list_next},                    /* Ntarget */
	{"list_child", cmd_list_child},                  /* Ntarget */
	{"list_nth", cmd_list_nth},                      /* Ntarget int */
	{"list_nthname", cmd_list_nthname},              /* Ntarget int name */
	{"list_del_nth", cmd_list_del_nth},              /* Nlist int */
	{"list_del_child", cmd_list_del_child},          /* Nlist Nchild */
	{"list_find_node", cmd_list_find_node},          /* Nlist, Nchild */
	{"table_cell", cmd_table_cell},                  /* Ntable line col */
	{"table_ins_row", cmd_table_ins_row},            /* Ntable int */
	{"table_ins_col", cmd_table_ins_col},            /* Ntable int */
	{"table_del_row", cmd_table_del_row},            /* Ntable int */
	{"table_del_col", cmd_table_del_col},            /* Ntable int */
	{"table_find_cell", cmd_table_find_cell},        /* Ntable, Nchild */
	{"table_detach_cell", cmd_table_detach_cell},    /* Ntable row col Nold */
	{"table_replace_cell", cmd_table_replace_cell},  /* Ntable row col Nsource Nold */
	{"table_detach_child", cmd_table_detach_child},  /* Ntable Nchild */
	{"tree_merge", cmd_tree_merge},                  /* Ndst Nsrc */
	{"path", cmd_path},                              /* Ndest str */
	{"path_nofollow", cmd_path_nofollow},            /* Ndest str */
	{"path_build", cmd_path_build},                  /* Ncwd Nnode */
	{"echo", cmd_echo},                              /* string */
	{"root", cmd_root},                              /* Ntarget Dsource */
	{"equ_node", cmd_equ_node},                      /* Ntarget Nsource */
	{"equ_doc",  cmd_equ_node},                      /* Dtarget Dsource */
	{"tree_has_symlink", cmd_tree_has_symlink},      /* Nsrc */
	{"symlink_is_broken", cmd_symlink_is_broken},    /* Nparent, Nsy */
	{"node_is_under", cmd_node_is_under},            /* Nsource, Nparent */
	{"list_replace_child", cmd_list_replace_child},  /* Nlist, Ncurrent, Nnew */
	{"list_detach_child", cmd_list_detach_child},    /* Nlist, Nnode */
	{"table_replace_child", cmd_table_replace_child},/* Ntable, Ncurrent, Nnew */
	{"tree_unlink", cmd_tree_unlink},                /* Nnode */
	{"tree_detach", cmd_tree_detach},                /* Nnode */
	{"list_detach_nth", cmd_list_detach_nth},        /* Nlist, int */
	{"table_named_cell", cmd_table_named_cell},      /* Ntable string int string int Ndest */
	{"dom_load", cmd_dom_load},                      /* Dtarget filename */
	{"dom_iterate", cmd_dom_iterate},                /* Nnode */
	{"dom_node_free", cmd_dom_node_free},            /* Nnode */
	{"dom_list_append", cmd_dom_list_append},        /* Nlist Nnode */
	{"dom_list_insert", cmd_dom_list_insert},        /* Nlist Nnode */
	{"dom_list_insert_after", cmd_dom_list_insert_after},/* Nlist Nnode */
	{"dom_list_len", cmd_dom_list_len},              /* Nlist */
	{"dom_hash_put", cmd_dom_hash_put},              /* Nhash Nnode */
	{"dom_hash_get", cmd_dom_hash_get},              /* Nhash Nnode key */
	{"tree_del", cmd_tree_del},                      /* Nnode */
	{"tree_dup", cmd_tree_dup},                      /* Nnode, Nnode */
	{NULL, NULL}
};

void command(char *line)
{
	char *args;
	cmd_t *c;

	args = strpbrk(line, " \t");
	if (args != NULL) {
		*args = '\0';
		args++;
	}
	shift_ws(&args);
	for(c = cmds; c->name != NULL; c++) {
		if (strcmp(c->name, line) == 0) {
			c->cmd(args);
			return;
		}
	}
	fprintf(stderr, "Invalid command %s\n", line);
	exit(1);
}

int main()
{
	lineno = 0;
	memset(doc, 0, sizeof(doc));
	memset(node, 0, sizeof(node));

	while(!(feof(stdin))) {
		char line[1024], *l, *end;
		lineno++;
		*line = '\0';
		fgets(line, sizeof(line)-1, stdin);
		l = line;
		while(isspace(*l)) l++;
		switch(*l) {
			case '\0':
			case '#':
			case '\n':
			case '\r':
				break;
			default:
				/* strip trailing whitespace */
				end = l + strlen(l) - 1;
				while((*end == '\r') || (*end == '\n')) {
					*end = '\0';
					if (end == l)
						break;
					end--;
				}
				command(l);
		}
	}

	return 0;
}
