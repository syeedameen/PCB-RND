/* generalized regex lib based on Ozan S. Yigit's implementation.
   Generalization done by  Tibor 'Igor2' Palinkas in 2012
   This file is placed in the Public Domain.
*/

#ifdef BIN_API
#define LINELEN ,linelen
#define SUBSLEN ,subslen
#else
#define LINELEN
#define SUBSLEN
#endif

int PRFX(main)()
{
	char line[1024];
	char subs[1024];
	int subslen;
	int linelen;
	PRFX(t) *r;

/* input header is match and subst in the first 2 lines */
	fgets(line, sizeof(line), stdin);
	strip(line);
	fgets(subs, sizeof(subs), stdin);

	/* compile regex */
	r = PRFX(comp)(line);
	if (PRFX(has_error)(r)) {
		/* use stdout instead of stderr: error message should be compared to the reference */
		printf("*** error: [%d] '%s'\n", PRFX(errno)(r), re_error_str(PRFX(errno)(r)));
		PRFX(free)(r);
		return 0;
	}

	/* clean up subs */
	strip(subs);
	if ((*subs == '\n') || (*subs == '\r'))
		*subs = '\0';
	subslen = strlen(subs);

	while(!(feof(stdin))) {
		*line = '\0';
		fgets(line, sizeof(line), stdin);
		strip(line);
		linelen=strlen(line);
#ifdef BIN_API
	line[linelen]='1';
#endif
		if ((PRFX(exec)(r, line LINELEN)) || (*subs != '\0')) {
			if (*subs != '\0') {
				char *dst;
				int len;
				if (backref)
					len = PRFX(backref)(r, &dst, subs SUBSLEN);
				else
					len = PRFX(subst)(r, &dst, line LINELEN, subs SUBSLEN, 0);
				if (dst == NULL) {
					dst = "<null>";
					len = 6;
				}
#ifdef BIN_API
				{
					char copy[8192];
					assert(len < sizeof(copy) - 1);
					memcpy(copy, dst, len);
					copy[len] = '\0';
					printf("%s\n", copy);
				}
#else
				printf("%s\n", dst);
#endif
			}
			else {
				/* no subs, just match; remove binary watchdog char from the end */
				line[linelen] = '\0';
				printf("%s\n", line);
			}
		}
	}

	PRFX(free)(r);
	return 0;
}
#undef LINELEN
#undef SUBSLEN
