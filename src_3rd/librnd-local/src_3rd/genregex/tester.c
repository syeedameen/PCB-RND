/* generalized regex lib based on Ozan S. Yigit's implementation.
   Generalization done by  Tibor 'Igor2' Palinkas in 2012
   This file is placed in the Public Domain.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void strip(char *s)
{
	char *end;
	
	end = s + strlen(s) - 1;
	while((end >= s) && ((*end == '\n') || (*end == '\r'))) {
		*end = '\0';
		end--;
	}
}

int backref = 0;

/* -- string -- */
#include "regex_se.h"
#define PRFX(name) re_se_ ## name
#include "tester_main.c"
#undef PRFX

#include "regex_sei.h"
#define PRFX(name) re_sei_ ## name
#include "tester_main.c"
#undef PRFX

#include "regex_st.h"
#define PRFX(name) re_st_ ## name
#include "tester_main.c"
#undef PRFX

#include "regex_sti.h"
#define PRFX(name) re_sti_ ## name
#include "tester_main.c"
#undef PRFX

/* -- binary -- */
#define BIN_API

#include "regex_be.h"
#define PRFX(name) re_be_ ## name
#include "tester_main.c"
#undef PRFX

#include "regex_bei.h"
#define PRFX(name) re_bei_ ## name
#include "tester_main.c"
#undef PRFX

#include "regex_bt.h"
#define PRFX(name) re_bt_ ## name
#include "tester_main.c"
#undef PRFX

#include "regex_bti.h"
#define PRFX(name) re_bti_ ## name
#include "tester_main.c"
#undef PRFX

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Need an argument: regex type: se, sei, be, bei, st, sti, bt or bti\n");
		exit(1);
	}
	if ((argv[2] != NULL) && (*argv[2] == 'b'))
		backref=1;
	if (strcmp(argv[1], "se") == 0)   return re_se_main();
	if (strcmp(argv[1], "sei") == 0)  return re_sei_main();
	if (strcmp(argv[1], "be") == 0)   return re_be_main();
	if (strcmp(argv[1], "bei") == 0)  return re_bei_main();
	if (strcmp(argv[1], "st") == 0)   return re_st_main();
	if (strcmp(argv[1], "sti") == 0)  return re_sti_main();
	if (strcmp(argv[1], "bt") == 0)   return re_bt_main();
	if (strcmp(argv[1], "bti") == 0)  return re_bti_main();
	fprintf(stderr, "Unknown regex type %s\n", argv[1]);
	return 1;
}
