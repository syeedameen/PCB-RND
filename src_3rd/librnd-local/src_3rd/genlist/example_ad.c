#include <stdio.h>
#include <string.h>
#include "genadlist.h"
#include "../regression/common_gadl.h"

int n;
item_t *i, *a, *b;
gdl_iterator_t it;

int main()
{
	list1 = gadl_list_init(NULL, sizeof(item_t), NULL, NULL);
	init_fill(list1, 5);
	dump_list(list1, "INIT");

	return 0;
}
