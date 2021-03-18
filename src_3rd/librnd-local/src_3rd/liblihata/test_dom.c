/* This file is placed in the public domain by its
   author Tibor 'Igor2' Palinkas in 2013. Feel free to
   copy and extend it in case you need DOM. */

#include <stdlib.h>
#include <stdio.h>
#include "dom.h"

int main()
{
	lht_doc_t *doc;
	int error = 0;

	doc = lht_dom_init();
	lht_dom_loc_newfile(doc, "<stdin>");
	while(!(feof(stdin))) {
		lht_err_t err;
		int c = getchar();
		err = lht_dom_parser_char(doc, c);
		if (err != LHTE_SUCCESS) {
			if (err != LHTE_STOP) {
				const char *fn;
				int line, col;
				lht_dom_loc_active(doc, &fn, &line, &col);
				printf("* error: %s (%s:%d.%d)*\n", lht_err_str(err), fn, line+1, col+1);
				error = 1;
			}
			break; /* error or stop, do not read anymore (would get LHTE_STOP without any processing all the time) */
		}
	}

	if (!error) {
		if (doc->root == NULL)
			printf("* empty document *\n");
		else
			lht_dom_ptree(doc->root, stdout, "");
	}

	lht_dom_uninit(doc);
	return 0;
}
