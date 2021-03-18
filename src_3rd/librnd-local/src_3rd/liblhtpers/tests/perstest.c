/*
    lhtpers - persistent lihata formatting
    Copyright (C) 2016  Tibor 'Igor2' Palinkas

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


    Persistent style save test program.
*/


#include <stdlib.h>
#include <string.h>
#include <liblhtpers/lhtpers.h>

#define PB_BEGIN     {"* ", 3, 3}
#define PB_BEGINNL   {"\n *", 4, 4}
#define PB_EMPTY     {"", 1, 1}
#define PB_SEMICOLON {";", 2, 2}
#define PB_SPACE     {" ", 2, 2}
#define PB_LBRACE    {"{", 2, 2}
#define PB_LBRACENL  {"{\n", 3, 3}
#define PB_LBRACEI   {"{\n *", 5, 5}
#define PB_RBRACE    {"}", 2, 2}
#define PB_RBRACENL  {"}\n", 3, 3}
#define PB_RBRACESC  {"};", 3, 3}
#define PB_NEWLINE   {"\n", 2, 2}
#define PB_DEFAULT   {NULL, 0, 0}

lht_perstyle_t style_inline = {
	/* buff */        {PB_SPACE, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_SEMICOLON},
	/* has_eq */      1,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

lht_perstyle_t style_newline = {
	/* buff */        {PB_BEGIN, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_NEWLINE},
	/* has_eq */      1,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

lht_perstyle_t style_istruct = {
	/* buff */        {PB_SPACE, PB_SPACE, PB_LBRACE, PB_EMPTY, PB_EMPTY, PB_RBRACESC},
	/* has_eq */      1,
	/* val_brace */   1,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

lht_perstyle_t style_struct = {
	/* buff */        {PB_BEGIN, PB_SPACE, PB_LBRACE, PB_EMPTY, PB_EMPTY, PB_RBRACENL},
	/* has_eq */      0,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

lht_perstyle_t style_structi = {
	/* buff */        {PB_BEGIN, PB_SPACE, PB_LBRACEI, PB_EMPTY, PB_EMPTY, PB_RBRACENL},
	/* has_eq */      0,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

lht_perstyle_t style_nlstruct = {
	/* buff */        {PB_BEGINNL, PB_SPACE, PB_LBRACENL, PB_EMPTY, PB_EMPTY, PB_RBRACENL},
	/* has_eq */      0,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

lhtpers_rule_t r_ilists[] = {
	{(const char *[]){"te:*", "ha:flags", "*", NULL},       &style_inline, NULL},
	{(const char *[]){"te:*", "ha:attributes", "*", NULL},  &style_newline, NULL},
	{NULL, NULL, NULL}
};

lhtpers_rule_t r_line[] = {
	{(const char *[]){"te:x1", "ha:line.*", "*", NULL},         &style_inline, NULL},
	{(const char *[]){"te:y1", "ha:line.*", "*", NULL},         &style_inline, NULL},
	{(const char *[]){"te:x2", "ha:line.*", "*", NULL},         &style_inline, NULL},
	{(const char *[]){"te:y2", "ha:line.*", "*", NULL},         &style_inline, NULL},
	{(const char *[]){"te:thickness", "ha:line.*", "*", NULL},  &style_inline, NULL},
	{(const char *[]){"te:clearance", "ha:line.*", "*", NULL},  &style_inline, NULL},
	{(const char *[]){"ha:flags", "*", "ha:line.*", NULL},      &style_istruct, NULL},
	{(const char *[]){"ha:attributes", "ha:line.*", "*", NULL}, &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

lhtpers_rule_t r_data[] = {
	{(const char *[]){"ha:line.*", "*", NULL}, &style_structi, r_line, NULL},
	{(const char *[]){"te:*", "ta:*", "*", NULL}, &style_inline, NULL, NULL},

	{NULL, NULL, NULL}
};

lhtpers_rule_t *rules[] = {
	r_data, r_ilists, r_line, NULL
};


static lhtpers_ev_res_t check_text(void *ev_ctx, lht_perstyle_t *style, lht_node_t *inmem_node, const char *ondisk_value)
{
	/* if node name starts with num, do a numeric comparison and keep on-disk format if the double values match */
	if (strncmp(inmem_node->name, "num", 3) == 0) {
		double mem= strtod(inmem_node->data.text.value, NULL), disk = strtod(ondisk_value, NULL);
		if (mem != disk)
			return LHTPERS_MEM;
	}
	if (strcmp(inmem_node->data.text.value, ondisk_value) != 0)
		return LHTPERS_MEM;

	return LHTPERS_DISK;
}

static lhtpers_ev_res_t check_list_empty(void *ev_ctx, lht_perstyle_t *style, lht_node_t *inmem_parent_node)
{
	return LHTPERS_INHIBIT;
}

static lhtpers_ev_res_t check_list_elem_removed(void *ev_ctx, lht_perstyle_t *style, lht_node_t *inmem_parent_node)
{
	return LHTPERS_INHIBIT;
}

int main(int argc, char *argv[])
{
	lht_doc_t *inp = NULL;
	lhtpers_ev_t events;
	char out[256];

	memset(&events, 0, sizeof(events));
	events.text = check_text;
	events.list_empty = check_list_empty;
	events.list_elem_removed = check_list_elem_removed;
	events.output_rules = rules;
	sprintf(out, "%s.out", argv[1]);

	if (argv[2] != NULL) {
		char *errmsg;
		inp = lht_dom_load(argv[2], &errmsg);
		if (inp == NULL) {
			fprintf(stderr, "Can't open input doc %s: %s\n", argv[2], errmsg);
			return 1;
		}
	}

	lht_err_t e = lhtpers_save_as(&events, inp, argv[1], out, NULL);
	return 0;
}
