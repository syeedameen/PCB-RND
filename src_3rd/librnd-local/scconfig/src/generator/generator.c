/*
    scconfig - minimalistic template system
    Copyright (C) 2009  Tibor Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

		Project page: http://repo.hu/projects/scconfig
		Contact via email: scconfig [at] igor2.repo.hu
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "db.h"
#include "log.h"
#include "libs.h"
#include "regex.h"
#include "../tmpasm/openfiles.h"
#include "generator.h"

#define separator '#'

typedef enum {
	cp_mute,   /* drop characters */
	cp_out,    /* copy characters on output */
	cp_buff    /* copy characters in an internal buffer */
} copymode_t;

typedef struct stack_s {
	copymode_t copy_mode;
	char *buff;
	int alloced, used;
	int nonest;
	char *forvar, *forvarin;
	FILE *in, *out;
	char *cmd;
	int sepcount;
} gen_stack_t;

/* Stack for nested ifs and fors. */
#define stack_max 128
static gen_stack_t stack[stack_max];
static int sp; /* points to the first free slot above the top of the stack */
#define TOP stack[sp-1]
static openfiles_t of;

static char cmd_buff[1024];
static char *cmd;
static int sepcount;

static void push()
{
	if (sp >= stack_max) {
		error("ifs are nested too deep\n");
		abort();
	}
	stack[sp].copy_mode = TOP.copy_mode;
	stack[sp].nonest    = TOP.nonest;
	stack[sp].buff      = NULL;
	stack[sp].alloced   = 0;
	stack[sp].used      = 0;
	stack[sp].forvar    = NULL;
	stack[sp].forvarin  = NULL;
	stack[sp].in        = TOP.in;
	stack[sp].out       = TOP.out;
	stack[sp].sepcount  = sepcount;
	*cmd = '\0';
	stack[sp].cmd       = strclone(cmd_buff);
	cmd = cmd_buff;
	*cmd = '\0';
	sp++;
}

static void pop()
{
	strcpy(cmd_buff, TOP.cmd);
	cmd = cmd_buff + strlen(cmd_buff);
	free(TOP.cmd);
	TOP.cmd = NULL;
	sepcount = TOP.sepcount;

	if (TOP.buff != NULL)
		free(TOP.buff);
	if (TOP.forvar != NULL)
		free(TOP.forvar);
	if (TOP.forvarin != NULL)
		free(TOP.forvarin);
	sp--;
	if (sp <= 0) {
		error("if underflow (too many endifs)\n");
		abort();
	}
}

genmode_t generate_gotchar(genmode_t mode, int c);

int exec_file(const char *input, const char *output);

/* generate some output: emit different objects */
void generator_emit_char(int c)
{
	switch(TOP.copy_mode) {
		case cp_mute:
			break;
		case cp_out:
			fputc(c, TOP.out);
			break;
		case cp_buff:
			if (TOP.used >= TOP.alloced) {
				TOP.alloced += 256;
				TOP.buff = realloc(TOP.buff, TOP.alloced);
			}
			TOP.buff[TOP.used] = c;
			TOP.used++;
			break;
	}
}

void generator_emit_string(const char *s)
{
	if (TOP.copy_mode == cp_mute)
		return;
	for(;*s != '\0'; s++)
		generator_emit_char(*s);
}

void generator_emit_separators(int count)
{
	int n;

	if (TOP.copy_mode == cp_mute)
		return;

	for(n = 0; n < count; n++)
		generator_emit_char(separator);
}

/* execute different commands */
static void do_put(char *cmd, int appending, int node, int eval)
{
	char *name, *value, *dvalue;
	const char *cvalue;

	name  = cmd;
	value = strchr(name, ' ');
	if (value == NULL) {
		error("no value for put or append");
		abort();
	}
	*value = '\0';
	value++;
	if (node) {
		cvalue = get(value);
		if (value == NULL)
			cvalue = "";
	}
	else
		cvalue = value;

	/* whether need to eval the result */
	if (eval) {
		dvalue = generator_eval(cvalue, mode_cmd);
		cvalue = dvalue;
	}
	else
		dvalue = NULL;

	if (appending) {
		if (cvalue != NULL)
			append(name, cvalue);
	}
	else
		put(name, cvalue);
	if (dvalue != NULL)
		free(dvalue);
}

static void do_report(char *cmd)
{
	report("%s\n", cmd);
}

static void do_abort(char *cmd)
{
	report("Abort requested by template.\n");
	abort();
}

static void do_if(char *cmd)
{
	char *left, *condition, *right, *end;
	const char *node;
	float node_val, right_val;
	int invert = 1, outcome = 0;
	int invalid_num = 0;


	left = cmd;
	condition = strchr(left, ' ');
	if (condition != NULL) {
		*condition = '\0';
		condition++;
		right = strchr(condition, ' ');
		if (right == NULL) {
			if (*condition != '@') {
				error("no right part of if with condition");
				abort();
			}
		}
		else {
			*right = '\0';
			right++;
		}
	}
	else
		right = NULL;

	while(*left == '!') {
		left++;
		invert = -invert;
	}

	node = get(left);
	if (node == NULL)
		node = "";
	node_val  = strtod(node, &end);
	if (*end != '\0') invalid_num++;
	if (right != NULL) {
		right_val = strtod(right, &end);
		if (*end != '\0') invalid_num++;
	}

	if (condition == NULL) {
		outcome = istrue(node);
	}
	else {
		if (invalid_num && ((*condition == '>') || (*condition == '<'))) {
			error("Invalid numeric format for if\n");
			abort();
		}
		switch(*condition) {
			case '=': break;
			case '@': outcome = (*node == '\0'); break;
			case '<': outcome = (node_val < right_val); break;
			case '>': outcome = (node_val > right_val); break;
			case '~':
				re_comp(right);
				outcome = re_exec(node);
				break;
			default:
				error("Illegal condition\n");
				abort();
		}
		if (condition[1] == '=') {
			outcome |= (strcmp(node, right) == 0);
			if (!invalid_num)
				outcome |= (node_val == right_val);
		}
	}
	if (invert < 0)
		outcome = !outcome;

	push();
	if (outcome)
		TOP.copy_mode = stack[sp-2].copy_mode;
	else
		TOP.copy_mode = cp_mute;
}

static void do_else(char *cmd)
{
	cmd = NULL; /* suppress compiler warning about unused arg */

	if (TOP.copy_mode == cp_mute)
		TOP.copy_mode = stack[sp-2].copy_mode;
	else
		TOP.copy_mode = cp_mute;
}

static void do_endif(char *cmd)
{
	cmd = NULL; /* suppress compiler warning about unused arg */

	pop();
}

static void do_get(char *cmd)
{
	const char *data;
	data = get(cmd);

	if (data != NULL)
		generator_emit_string(data);

}

static void do_foreach(char *cmd)
{
	char *forvar;
	char *forvarin;

	forvar = cmd;
	while((*forvar == ' ') || (*forvar == '\t')) forvar++;
	forvarin = strchr(forvar, ' ');
	if (forvarin != NULL) {
		*forvarin = '\0';
		forvarin++;
		while((*forvarin == ' ') || (*forvarin == '\t')) forvarin++;
	}
	else {
		fprintf(stderr, "Error: foreach: not enough arguments\n");
		abort();
	}

	/* start saving bytes into a new buffer */
	push();
	TOP.copy_mode = cp_buff;
	TOP.nonest = 1;

	/* remember settings for do_done */
	TOP.forvar = strclone(forvar);
	TOP.forvarin = strclone(forvarin);
}

static void do_done(char *cmd)
{
	genmode_t mode;
	char *b, *s, *next;
	const char *cs;
	cmd = NULL; /* suppress compiler warning about unused arg */

	/* run the loop */
	TOP.buff[TOP.used] = '\0';
	cs = get(TOP.forvarin);
	if (cs != NULL) {
		while(*cs == ' ') cs++;
		s = strclone(cs);
		do {
			next = strchr(s, ' ');
			if (next != NULL) {
				*next = '\0';
				next++;
				while(*next == ' ') next++;
			}
			put(TOP.forvar, s);
			b = TOP.buff;
			push();
			mode          = mode_copy;
			TOP.nonest    = 0;
			TOP.copy_mode = stack[sp-3].copy_mode;
			for(; *b != '\0'; b++) {
				mode = generate_gotchar(mode, *b);
				if (mode == mode_fatal)
					break;
			}
			pop();
			s = next;
		} while(s != NULL);
		free(s);
	}
	pop();
}

static void do_include(char *cmd, int node)
{
	FILE *in;
	const char *fn;

	while((*cmd == ' ') || (*cmd == '\t')) cmd++;
	if (node) {
		fn = get(cmd);
		if (fn == NULL)
			fn = "";
	}
	else
		fn = cmd;

	in = openfile_open(&of, fn, "r");
	if (in != NULL) {
		rewind(in);
		push();
		TOP.in = in;
		TOP.out = stack[sp-2].out;
		exec_file(NULL, NULL);
		pop();
	}
	else
		report("Error: generator: can't open %s for reading (for include)\n", fn);
}


static void do_sub(char *cmd, int global, int subnode)
{
	char *node, *pat, *sub, *err, *start;
	char *buff, *end;
	const char *val, *csub;
	int score, slen;

	node = cmd;
	pat = strchr(cmd, ' ');
	if (pat == NULL) {
		fprintf(stderr, "Error: generator: no node argument for sub\n"); /* TODO: line number! */
		return;
	}
	*pat = '\0';
	pat++;

	sub = pat;
	while(1) {
		sub = strchr(sub, ' ');
		if (sub == NULL) {
			fprintf(stderr, "Error: generator: no substitution string argument for sub\n"); /* TODO: line number! */
			return;
		}
		if (sub[-1] != '\\') {
			*sub = '\0';
			sub++;
			break;
		}
		sub++;
	}

	val = get(node);
	if (val == NULL)
		val="";
	err = re_comp(pat);
	if (err != NULL) {
		fprintf(stderr, "Error: generator: regex syntax error: %s\n", err); /* TODO: line number! */
		return;
	}

	if (subnode) {
		csub = (char *)get(sub);
		if (sub == NULL) {
			fprintf(stderr, "Error: generator: invalid second node for nsub/ngsub\n"); /* TODO: line number! */
			return;
		}
	}
	else
		csub = sub;

	slen = strlen(csub);
	if (global)
		buff = malloc(strlen(val)*(slen+3)+32); /* big enough for worst case, when every letter and $ and ^ are replaced with sub */
	else
		buff = malloc(strlen(val)+slen+32);  /* only one replacement will be done */
	strcpy(buff, val);


	start = buff;
	do {
		score = re_exec(start);
		if (score == 0)
			break;
		end = buff + strlen(buff);
		if (eopat[0] - bopat[0] != slen) {
			int mlen = end - eopat[0]+1;
			if (mlen > 0)
				memmove(bopat[0] + slen, eopat[0], mlen);
		}
		memcpy(bopat[0], csub, slen);
		start = bopat[0] + slen;
	} while(global);

	generator_emit_string(buff);
	free(buff);
}

static void do_redir(char *cmd)
{
	FILE *out;
	if (*cmd == '\0') {
		pop();
		return;
	}
	while((*cmd == ' ') || (*cmd == '\t')) cmd++;
	push();
	out = openfile_open(&of, cmd, "w");
	if (out == NULL)
		report("Error: generator: can't open %s for writing (for redirection)\n", cmd);
	else
		TOP.out = out;
}

static void do_neval(char *cmd)
{
	char *res;

	res = generator_eval(get(cmd), mode_copy);
	if (res != NULL) {
		generator_emit_string(res);
		free(res);
	}
}

static void do_nsplit(char *cmd)
{
	const char *s;
	char *str, *arr, *regex, *val, *path, *idx;
	int len, i;

	str = cmd;
	arr = strpbrk(str, " \t");
	if (arr == NULL)
		goto nsplit_err;
	*arr = '\0';
	arr++;
	regex = strpbrk(arr, " \t");
	if (regex == NULL)
		goto nsplit_err;
	*regex = '\0';
	regex++;

	s = get(str);
	re_comp(regex);
	path = malloc(strlen(arr) + 32);
	strcpy(path, arr);
	idx = path + strlen(arr);
	*idx = '/';
	idx++;
	i = 1;
	while((s != NULL) && (re_exec(s) > 0)) {
		len = bopat[0] - s;
		val = malloc(len + 2);
		strncpy(val, s, len);
		val[len] = '\0';
		sprintf(idx, "%d", i);
		put(path, val);
		free(val);
		s = eopat[0];
		i++;
	};
	sprintf(idx, "%d", i);
	put(path, s);

	return;
	nsplit_err:;
	report("Error: generator: invalid number of arguments for nsplit: %s\n", cmd);
}

static void do_uniq(char *cmd)
{
	char *sep, *node;
	char **list;
	int used, n;

	node = strpbrk(cmd, " \t");
	*node = '\0';
	node++;
	if (node == NULL)
		return;

	sep = esc_interpret(cmd);

	list = uniq_inc_arr(node, 1, sep, &used);

	/* print list */
	for(n = 0; n < used; n++) {
		if (n > 0)
			generator_emit_char(*sep);
		generator_emit_string(list[n]);
	}
	uniq_inc_free(list);
}


#ifdef GENCALL
extern void generator_callback(char *cmd, char *args);

static void do_call(char *cmd)
{
	char *args;

	args = strpbrk(cmd, " \t");
	if (args != NULL) {
		*args = '\0';
		args++;
	}
	generator_callback(cmd, args);
}
#endif


/* the command dispatcher */
static void do_cmd(char *cmd)
{
#define CLOSE \
	if ((TOP.nonest == 0) || ((--TOP.nonest) == 0)) {

#define ELSE \
	} else goto pass;

#define OPEN \
	if (TOP.nonest != 0) {\
		TOP.nonest++; \
		goto pass; \
	} \
	else


#define NEUTRAL \
	if (TOP.nonest != 0) goto pass; \
	if (TOP.copy_mode != cp_mute)


#define NEUTRAL_IF \
	if (TOP.nonest != 0) goto pass; \

	     if (strncmp(cmd, "endif",     5) == 0) { CLOSE   do_endif(cmd+5); ELSE }
	else if (strncmp(cmd, "done",      4) == 0) { CLOSE   do_done(cmd+4); ELSE }
	else if (strncmp(cmd, "foreach ",  8) == 0) { OPEN    do_foreach(cmd+8); }
	else if (strncmp(cmd, "if ",       3) == 0) { OPEN    do_if(cmd+3); }
	else if (strncmp(cmd, "report ",   7) == 0) { NEUTRAL do_report(cmd+7); }
	else if (strncmp(cmd, "abort",     5) == 0) { NEUTRAL do_abort(cmd+5); }
	else if (strncmp(cmd, "sep",       3) == 0) { NEUTRAL generator_emit_separators(3); }
	else if (strncmp(cmd, "redir",     5) == 0) { NEUTRAL do_redir(cmd+5); }
	else if (strncmp(cmd, "include ",  8) == 0) { NEUTRAL do_include(cmd+8, 0); }
	else if (strncmp(cmd, "ninclude ", 9) == 0) { NEUTRAL do_include(cmd+8, 1); }
	else if (strncmp(cmd, "sep",       3) == 0) { NEUTRAL generator_emit_separators(3); }
	else if (strncmp(cmd, "neval ",    6) == 0) { NEUTRAL do_neval(cmd+6); }
	else if (strncmp(cmd, "nsplit ",   7) == 0) { NEUTRAL do_nsplit(cmd+7); }
	else if (strncmp(cmd, "put ",      4) == 0) { NEUTRAL do_put(cmd+4, 0, 0, 0); }
	else if (strncmp(cmd, "append ",   7) == 0) { NEUTRAL do_put(cmd+7, 1, 0, 0); }
	else if (strncmp(cmd, "nput ",     5) == 0) { NEUTRAL do_put(cmd+5, 0, 1, 0); }
	else if (strncmp(cmd, "nappend ",  8) == 0) { NEUTRAL do_put(cmd+8, 1, 1, 0); }
	else if (strncmp(cmd, "puteval ",  8) == 0) { NEUTRAL do_put(cmd+8, 0, 0, 1); }
	else if (strncmp(cmd, "appendeval ",11) == 0) { NEUTRAL do_put(cmd+11, 1, 0, 1); }
	else if (strncmp(cmd, "nputeval ", 9) == 0) { NEUTRAL do_put(cmd+9, 0, 1, 1); }
	else if (strncmp(cmd, "nappendeval ",12) == 0) { NEUTRAL do_put(cmd+12, 1, 1, 1); }
	else if (strncmp(cmd, "sub ",      4) == 0) { NEUTRAL do_sub(cmd+4, 0, 0); }
	else if (strncmp(cmd, "gsub ",     5) == 0) { NEUTRAL do_sub(cmd+5, 1, 0); }
	else if (strncmp(cmd, "nsub ",     5) == 0) { NEUTRAL do_sub(cmd+5, 0, 1); }
	else if (strncmp(cmd, "ngsub ",    6) == 0) { NEUTRAL do_sub(cmd+6, 1, 1); }
	else if (strncmp(cmd, "uniq ",     5) == 0) { NEUTRAL do_uniq(cmd+5); }
#ifdef GENCALL
	else if (strncmp(cmd, "call ",     5) == 0) { NEUTRAL do_call(cmd+5); }
#endif
	else if (strncmp(cmd, "else",      4) == 0) { NEUTRAL_IF do_else(cmd+5); }
	else                                       { NEUTRAL do_get(cmd); }

	/* falling trough here means one of the do_ ran */
	return;
	pass:;
		generator_emit_separators(3);
		generator_emit_string(cmd);
		generator_emit_separators(3);

#undef CLOSE
#undef OPEN
}


genmode_t generate_gotchar(genmode_t mode, int c)
{
		switch(mode) {
			case mode_copy:
				if (c == separator) {
					sepcount = 1;
					mode = mode_copy_sep;
				}
				else
					generator_emit_char(c);
				break;
			case mode_cmd:
				if (c == separator) {
					sepcount = 1;
					mode = mode_cmd_sep;
				}
				else {
					*cmd = c;
					cmd++;
					if (cmd - cmd_buff >= (int)sizeof(cmd_buff)) {
						/* no room for the terminator */
						return mode_fatal;
					}
				}
				break;
			case mode_copy_sep:
				if (c == separator) {
					sepcount++;
					if (sepcount == 3) {
						cmd  = cmd_buff;
						mode = mode_cmd;
					}
				}
				else {
					/* one or two hashmarks then something else */
					generator_emit_separators(sepcount);
					mode = mode_copy_sep;
					generator_emit_char(c);
					sepcount = 0;
				}
				break;
			case mode_cmd_sep:
				if (c == separator) {
					sepcount++;
					if (sepcount == 3) {
						*cmd = '\0';
						do_cmd(cmd_buff);
						mode = mode_copy;
					}
				}
				else {
					/* one or two separators then something else */
					generator_emit_separators(sepcount);
					mode = mode_copy_sep;
					generator_emit_char(c);
					sepcount = 0;
				}
				break;
			case mode_fatal:
				fprintf(stderr, "scconfig generator: internal error in parser state machine (executed mode_fatal)\n");
				abort();
		}
	return mode;
}

static int execute()
{
	genmode_t mode;
	int c;

	mode = mode_copy;

	while((c = fgetc(TOP.in)) != EOF) {
		mode = generate_gotchar(mode, c);
		if (mode == mode_fatal) {
			return -3;
		}
	}
	return 0;
}


/* generate an output file from an input file */
int exec_file(const char *input, const char *output)
{
	int ret, startsp;

	startsp = sp;

	if (input != NULL) {
		TOP.in = openfile_open(&of, input, "r");
		if (TOP.in == NULL)
			return -1;
	}

	if (output != NULL) {
		TOP.out = openfile_open(&of, output, "w");
		if (TOP.out == NULL) {
			fclose(TOP.in);
			return -1;
		}
	}

	ret = execute();

	if (sp != startsp) {
		error("Error in nesting: not enough endif or done tags\n");
		return -2;
	}
	return 0;
}

/* wrapper for setting up stack */
int generate(const char *input, const char *output)
{
	int ret;

	sp            = 1;
	TOP.copy_mode = cp_out;
	TOP.buff      = NULL;
	TOP.alloced   = 0;
	TOP.used      = 0;
	TOP.nonest    = 0;
	TOP.forvar    = NULL;
	TOP.forvarin  = NULL;

	memset(&of, 0, sizeof(of));
	ret = exec_file(input, output);
	openfile_free(&of);

	return ret;
}

char *generator_eval(const char *input, genmode_t start_mode)
{
	char *res;
	const char *b;
	genmode_t mode;

	if (input == NULL)
		return NULL;

	mode = start_mode;
	push();
	TOP.copy_mode = cp_buff;
	TOP.nonest    = 0;
	for(b = input; *b != '\0'; b++) {
		mode = generate_gotchar(mode, *b);
		if (mode == mode_fatal)
			break;
	}
	if ((start_mode == mode_cmd) && (mode == mode_cmd)) {
		mode = generate_gotchar(mode, separator);
		mode = generate_gotchar(mode, separator);
		mode = generate_gotchar(mode, separator);
	}
	generator_emit_char('\0');
	res = TOP.buff;
	TOP.buff = NULL;
	pop();
	return res;
}
