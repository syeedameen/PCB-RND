/* This file is placed in the public domain by its
   author Tibor 'Igor2' Palinkas in 2013. Feel free to
   copy and extend it in case you need event based lihata
   parsing. */

#include <stdlib.h>
#include <stdio.h>
#include "parser.h"

static void spaces(int num)
{
	for(;num;num--)
		putchar(' ');
}

void event(lht_parse_t *ctx, lht_event_t ev, lht_node_type_t nt, const char *name, const char *value)
{
	int *level = ctx->user_data;

	switch(ev) {
		case LHT_OPEN:
			spaces(*level);
			printf("%s %s\n", lht_type_name(nt), name);
			(*level)++;
			break;
		case LHT_CLOSE:
			(*level)--;
			break;
		case LHT_TEXTDATA:
			spaces(*level);
			printf("%s '%s' = '%s'\n", lht_type_name(nt), name, value);
			break;
		case LHT_COMMENT:
			spaces(*level);
			printf("# '%s'\n", name);
			break;
		case LHT_EOF:
			spaces(*level);
			printf("*EOF*\n");
			break;
		case LHT_ERROR:
			spaces(*level);
			printf("*ERROR* '%s' (at %d:%d)\n", name, ctx->line+1, ctx->col+1);
			break;
	}
}

int main()
{
	lht_parse_t p;
	int level = 0;

	lht_parser_init(&p);
	p.event = event;
	p.user_data = &level;

	while(!(feof(stdin))) {
		int c = getchar();
		if (lht_parser_char(&p, c))
			break; /* error or stop, do not read anymore (would get LHTE_STOP without any processing all the time) */
	}
	lht_parser_uninit(&p);
	return 0;
}
