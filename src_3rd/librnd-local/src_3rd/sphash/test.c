#include <stdio.h>
#include <string.h>
#include "test_hash.h"

int main()
{
	char s[128];
	while(!(feof(stdin))) {
		*s = '\0';
		fgets(s, sizeof(s), stdin);
		s[strlen(s)-1] = '\0';
		printf("'%s' is %d is '%s'\n", s, test_sphash(s), test_keyname(test_sphash(s)));
	}
	return 0;
}
