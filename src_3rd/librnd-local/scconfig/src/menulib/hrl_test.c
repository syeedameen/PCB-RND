#include <stdlib.h>
#include <string.h>
#include "hookreadline.h"

static int dummy_complete(hookreadline_t *h, char **line, int *cursor)
{
	char insert[32]; /* large enough for integer in decimal */
	char *newline;
	char c;
	static int ctr = 0;

	newline = malloc(strlen(*line) + 16);
	c = (*line)[*cursor];
	(*line)[*cursor] = '\0';
	sprintf(insert, "%d", ctr++);
	sprintf(newline, "%s%s%c%s", *line, insert, c, (*line)+*cursor+1);
	free(*line);
	(*line) = newline;
	(*cursor) += strlen(insert);
	return 1;
}

int main()
{
	hookreadline_t h;
	char *l;
	int suspcnt = 0;

	hrl_default(&h);
	hrl_calibrate(&h);
	fprintf(stderr, "size row=%d col=%d\n", h.term.maxrow, h.term.maxcol);
	h.complete = dummy_complete;
	h.prompt = malloc(16); /* strdup() is not portable */
	strcpy(h.prompt, "hrl-test> ");

	for(;;) {
		l = hrl_readline(&h, HRL_READLINE);
		if (l == NULL) {
			if (hrl_suspended(&h)) {
				vt100_move(&h.term, 20,20);
				printf("susp %d\n", suspcnt++);
			}
			else {
				printf("\nEOF\n");
				break;
			}
		}
		printf("\nline: '%s'\n", l);
	}
	return 0;
}
