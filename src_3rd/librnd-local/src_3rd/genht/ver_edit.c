#include <stdio.h>
#include <string.h>

#define replace(pattern, value) \
do { \
	char *next, *s = strstr(line, pattern); \
	if (s != NULL) { \
		next = s + strlen(pattern); \
		strcpy(s, value); \
		s += strlen(value); \
		memmove(s, next, strlen(next)+1); \
	} \
} while(0)

int main(int argc, char *argv[])
{
	char *major = argv[1], *minor = argv[2], *patch = argv[3], line[256];;
	while(fgets(line, sizeof(line), stdin) != NULL) {
		replace("_VER_MAJOR_", major);
		replace("_VER_MINOR_", minor);
		replace("_VER_PATCH_", patch);
		printf("%s", line);
	}
	return 0;
}
