#include <stdio.h>
#include <string.h>
#include "gendlist.h"
#include "../regression/common_gdl.h"

int n;
item_t *i, *a, *b;
gdl_iterator_t it;

int main()
{
	init_fill(&list1, 5);
	dump_list1("INIT");

	return 0;
}
