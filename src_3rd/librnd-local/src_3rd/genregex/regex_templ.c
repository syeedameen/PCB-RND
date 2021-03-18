/*
 * regex - Regular expression pattern matching  and replacement
 *
 * By:  Ozan S. Yigit (oz)
 *      Dept. of Computer Science
 *      York University
 *
 * These routines are the PUBLIC DOMAIN equivalents of regex
 * routines as found in 4.nBSD UN*X, with minor extensions.
 *
 * These routines are derived from various implementations found
 * in software tools books, and Conroy's grep. They are NOT derived
 * from licensed/restricted software.
 * For more interesting/academic/complicated implementations,
 * see Henry Spencer's regexp routines, or GNU Emacs pattern
 * matching module.
 *
 * const correctness patch by Tibor 'Igor2' Palinkas in 2009..2010
 * tailored to minetd use by Tibor 'Igor2' Palinkas in 2011
 * generalized for genregex by Tibor 'Igor2' Palinkas in 2012
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "regex_templ.h"
#include "regex.h"

#if RE_BIN_API == 1
#define SRCLEN , int srclen
#define BIN_OR_EMPTY(p)  , p
#define PATLEN , int patlen
#define PATLEN_ , patlen
#else
#define SRCLEN
#define BIN_OR_EMPTY(p)
#define PATLEN
#define PATLEN_
#endif

#if RE_ICASE == 1
#	include <ctype.h>
#	define re_tolower(c) tolower(c)
#else
#	define re_tolower(c) c
#endif

#define MAXTAG  10

enum opcode_e {
	NOP = 0,
	END = 0,

	CHR = 1,   /* specific char         */
	ANY = 2,   /* any character:   .    */
	CCL = 3,   /* character class: []   */
	BOL = 4,   /* begin-of-line:   ^    */
	EOL = 5,   /* end-of-line:     $    */
	BOT = 6,   /* begin-of-tag:    \(   */
	EOT = 7,   /* end-of-tag:      \)   */
	BOW = 8,   /* begin-of-word:   \<   */
	EOW = 9,   /* end-of-word:     \>   */
	REF = 10,  /* backref:         \1   */
	CLO = 11   /* closure               */
};

/*
 * The following defines are not meant to be changeable.
 * They are for readability only.
 */
#define MAXCHR	128
#define CHRBIT	8
#define BITBLK	MAXCHR/CHRBIT
#define BLKIND	0170
#define BITIND	07

#define ASCIIB	0177

#ifdef NO_UCHAR
typedef char CHAR;
#else
typedef unsigned char CHAR;
#endif

static CHAR bitarr[] = {1,2,4,8,16,32,64,128};

struct RE_PRFX(s) {
	CHAR *nfa;                        /* precompiled regex bytecode */
	int nfa_used, nfa_alloced;

	CHAR bittab[BITBLK];             /* bit table for CCL */
                                   /* pre-set bits...   */

	/* tags */
	const char *bol;
	const char *bopat[MAXTAG];
	const char *eopat[MAXTAG];

	/* dest buffer for subst and backref */
	char *dst;
	int dst_used, dst_alloced;

	/* error details */
	re_error_t error;
	char fail_op;
};

static void
chset(RE_PRFX(t) *r, CHAR c)
{
	r->bittab[(CHAR) ((c) & BLKIND) >> 3] |= bitarr[(c) & BITIND];
}

#define badpat(x)  do { r->error = x; goto error; } while(0)

#define grow(r, buf, delta) \
	if (r->buf ## _used + delta >= r->buf ## _alloced) { \
		r->buf ## _alloced += 256 + delta; \
		r->buf = realloc(r->buf, r->buf ## _alloced); \
	}

#define store(x) \
	do { \
		grow(r, nfa, 1); \
		*mp++ = x; \
	} while(0)



RE_PRFX(t) *RE_PRFX(comp)(const char *pat)
{
	int  tagstk[MAXTAG];             /* subpat tag stack..*/
	int  sta;                        /* status of lastpat */

	register const char *p;         /* pattern pointer   */
	register CHAR *mp;              /* nfa pointer       */
	register CHAR *lp;              /* saved pointer..   */
	register CHAR *sp;              /* another one..     */

	register int tagi = 0;          /* tag stack index   */
	register int tagc = 1;          /* actual tag count  */

	register int n;
	register CHAR mask;             /* xor mask -CCL/NCL */
	int c1, c2;

	RE_PRFX(t) *r;


	r = calloc(1, sizeof(RE_PRFX(t)));
	sta = NOP;
	grow(r, nfa, 1);
	sp = mp = r->nfa;

	if (!pat || !*pat) {
		if (sta)
			return NULL;
		else
			badpat(RE_ERR_NOPREV);
	}
	sta = NOP;

	for (p = pat; *p; p++) {
		lp = mp;
		switch(*p) {

		case '.':               /* match any char..  */
			store(ANY);
			break;

		case '^':               /* match beginning.. */
			if (p == pat)
				store(BOL);
			else {
				store(CHR);
				store(re_tolower(*p));
			}
			break;

		case '$':               /* match endofline.. */
			if (!*(p+1))
				store(EOL);
			else {
				store(CHR);
				store(re_tolower(*p));
			}
			break;

		case '[':               /* match char class..*/
			store(CCL);

			if (*++p == '^') {
				mask = 0377;
				p++;
			}
			else
				mask = 0;

			if (*p == '-')        /* real dash */
				chset(r, *p++);
			if (*p == ']')        /* real brac */
				chset(r, *p++);
			while (*p && *p != ']') {
				if (*p == '-' && *(p+1) && *(p+1) != ']') {
					p++;
					c1 = *(p-2) + 1;
					c2 = *p++;
					while (c1 <= c2)
						chset(r, re_tolower((CHAR)c1++));
				}
#if RE_EXTEND == 1
				else if (*p == '\\') {
					p++;
					switch (*p) {
						case 'n': chset(r, '\n'); break;
						case 'r': chset(r, '\r'); break;
						case 't': chset(r, '\t'); break;
						case '\\': chset(r, '\\'); break;
						case '\0': p--; chset(r, '\\'); break;
						default:  chset(r, *p);
					}
					p++;
				}
#endif
				else
					chset(r, re_tolower(*p++));
			}
			if (!*p)
				badpat(RE_ERR_NOCLCL);

			for (n = 0; n < BITBLK; r->bittab[n++] = (char) 0)
				store(mask ^ r->bittab[n]);
	
			break;

		case '*':               /* match 0 or more.. */
		case '+':               /* match 1 or more.. */
			if (p == pat)
				badpat(RE_ERR_EMPTYCLO);
			lp = sp;              /* previous opcode */
			if (*lp == CLO)       /* equivalence..   */
				break;
			switch(*lp) {

			case BOL:
			case BOT:
			case EOT:
			case BOW:
			case EOW:
			case REF:
				badpat(RE_ERR_BADCLO);
			default:
				break;
			}

			if (*p == '+')
				for (sp = mp; lp < sp; lp++)
					store(*lp);

			store(END);
			store(END);
			sp = mp;
			while (--mp > lp)
				*mp = mp[-1];
			store(CLO);
			mp = sp;
			break;

		case '\\':              /* tags, backrefs .. */
			switch(*++p) {

			case '(':
				if (tagc < MAXTAG) {
					tagstk[++tagi] = tagc;
					store(BOT);
					store(tagc++);
				}
				else
					badpat(RE_ERR_STACK);
				break;
			case ')':
				if (*sp == BOT)
					badpat(RE_ERR_EMPTYTAG);
				if (tagi > 0) {
					store(EOT);
					store(tagstk[tagi--]);
				}
				else
					badpat(RE_ERR_UNMATCHTAGC);
				break;
			case '<':
				store(BOW);
				break;
			case '>':
				if (*sp == BOW)
					badpat(RE_ERR_EMPTYWORD);
				store(EOW);
				break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				n = *p-'0';
				if (tagi > 0 && tagstk[tagi] == n)
					badpat(RE_ERR_CYCREF);
				if (tagc > n) {
					store(REF);
					store(n);
				}
				else
					badpat(RE_ERR_UNTERMREF);
				break;
#if RE_EXTEND == 1
			case 'b':
				store(CHR);
				store('\b');
				break;
			case 'n':
				store(CHR);
				store('\n');
				break;
			case 'f':
				store(CHR);
				store('\f');
				break;
			case 'r':
				store(CHR);
				store('\r');
				break;
			case 't':
				store(CHR);
				store('\t');
				break;
#endif
			default:
				store(CHR);
				store(re_tolower(*p));
			}
			break;

		default :               /* an ordinary char  */
			store(CHR);
			store(re_tolower(*p));
			break;
		}
		sp = lp;
	}
	if (tagi > 0)
		badpat(RE_ERR_UNMATCHTAGO);
	store(END);

	return r;
error:;
	return r;
}


static const char *pmatch(RE_PRFX(t) *r, const char *, const char *, CHAR *, int *);

#if RE_BIN_API == 1
#define end(s)   ((s) >= lpend)
#else
#define end(s)   (*(s) == '\0')
#endif
/*
 * re_exec:
 * 	execute nfa to find a match.
 *
 *	special cases: (nfa[0])
 *		BOL
 *			Match only once, starting from the
 *			beginning.
 *		CHR
 *			First locate the character without
 *			calling pmatch, and if found, call
 *			pmatch for the remaining string.
 *		END
 *			re_comp failed, poor luser did not
 *			check for it. Fail fast.
 *
 *	If a match is found, bopat[0] and eopat[0] are set
 *	to the beginning and the end of the matched fragment,
 *	respectively.
 *
 */

int RE_PRFX(exec)(RE_PRFX(t) *r, const char *lp SRCLEN)
{
	register CHAR c;
	register const char *ep = 0;
	register CHAR *ap = r->nfa;
	int score = 1;
#if RE_BIN_API == 1
	const char *lpend = lp + srclen;
#else
	const char *lpend = NULL;
#endif

	r->bol = lp;

	r->bopat[0] = 0;
	r->bopat[1] = 0;
	r->bopat[2] = 0;
	r->bopat[3] = 0;
	r->bopat[4] = 0;
	r->bopat[5] = 0;
	r->bopat[6] = 0;
	r->bopat[7] = 0;
	r->bopat[8] = 0;
	r->bopat[9] = 0;
	r->error = 0;
	r->fail_op = 0;

	switch(*ap) {

	case BOL:                 /* anchored: match from BOL only */
		ep = pmatch(r, lp, lpend, ap, &score);
		break;
	case CHR:                 /* ordinary char: locate it fast */
		c = *(ap+1);
		while (!end(lp) && re_tolower(*lp) != c)
			lp++;
		if (end(lp))            /* if EOS, fail, else fall thru. */
			return 0;
	default:                  /* regular matching all the way. */
		do {
			if ((ep = pmatch(r, lp, lpend, ap, &score)))
				break;
#if RE_BIN_API == 1
		} while (!end(++lp));
#else
		} while (!end(lp++));
#endif

		break;
	case END:                 /* munged automaton. fail always */
		return 0;
	}
	if (!ep)
		return 0;

	r->bopat[0] = lp;
	r->eopat[0] = ep;
	return score;
}

/*
 * pmatch: internal routine for the hard part
 *
 *	This code is partly snarfed from an early grep written by
 *	David Conroy. The backref and tag stuff, and various other
 *	innovations are by oz.
 *
 *	special case optimizations: (nfa[n], nfa[n+1])
 *		CLO ANY
 *			We KNOW .* will match everything upto the
 *			end of line. Thus, directly go to the end of
 *			line, without recursive pmatch calls. As in
 *			the other closure cases, the remaining pattern
 *			must be matched by moving backwards on the
 *			string recursively, to find a match for xy
 *			(x is ".*" and y is the remaining pattern)
 *			where the match satisfies the LONGEST match for
 *			x followed by a match for y.
 *		CLO CHR
 *			We can again scan the string forward for the
 *			single char and at the point of failure, we
 *			execute the remaining nfa recursively, same as
 *			above.
 *
 *	At the end of a successful match, bopat[n] and eopat[n]
 *	are set to the beginning and end of subpatterns matched
 *	by tagged expressions (n = 1 to 9).
 *
 */

/*
 * character classification table for word boundary operators BOW
 * and EOW. the reason for not using ctype macros is that we can
 * let the user add into our own table. see re_modw. This table
 * is not in the bitset form, since we may wish to extend it in the
 * future for other character classifications. 
 *
 *	TRUE for 0-9 A-Z a-z _
 */
static CHAR chrtyp[MAXCHR] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 0, 0, 0, 0, 1, 0, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 0, 0, 0, 0, 0
};

#define inascii(x)    (0177&(x))
#define iswordc(x)    chrtyp[inascii(x)]
#define isinset(x,y)  ((x)[((y)&BLKIND)>>3] & bitarr[(y)&BITIND])

/*
 * skip values for CLO XXX to skip past the closure
 */

#define ANYSKIP 2   /* [CLO] ANY END ...         */
#define CHRSKIP 3   /* [CLO] CHR chr END ...     */
#define CCLSKIP 18  /* [CLO] CCL 16bytes END ... */

static const char *pmatch(RE_PRFX(t) *r, const char *lp, const char *lpend, CHAR *ap, int *score)
{
	register int op, c, n;
	register const char *e;       /* extra pointer for CLO */
	register const char *bp;      /* beginning of subpat.. */
	register const char *ep;      /* ending of subpat.. */
	const char *are;              /* to save the line ptr. */

	while ((op = *ap++) != END)
		switch(op) {

		case CHR:
			if (re_tolower(*lp++) != *ap++)
				return 0;
			(*score) += 100;
			break;
		case ANY:
			if (end(lp++))
				return 0;
			(*score)++;
			break;
		case CCL:
			if (end(lp))
				return 0;
			c = re_tolower(*lp++);
			if (!isinset(ap,c))
				return 0;
			ap += BITBLK;
			(*score) += 2;
			break;
		case BOL:
			if (lp != r->bol)
				return 0;
			(*score) += 10;
			break;
		case EOL:
			if (!end(lp))
				return 0;
			(*score) += 10;
			break;
		case BOT:
			r->bopat[*ap++] = lp;
			break;
		case EOT:
			r->eopat[*ap++] = lp;
			break;
 		case BOW:
			if ((lp!=r->bol && iswordc(lp[-1])) || !iswordc(*lp))
				return 0;
			(*score) += 5;
			break;
		case EOW:
			if (lp==r->bol || !iswordc(lp[-1]) || (!end(lp) && iswordc(*lp)))
				return 0;
			(*score) += 5;
			break;
		case REF:
			n = *ap++;
			bp = r->bopat[n];
			ep = r->eopat[n];
			while (bp < ep) {
				if (*bp++ != *lp++)
					return 0;
			(*score) += 2;
			}
			break;
		case CLO:
			are = lp;
			switch(*ap) {

			case ANY:
				while (!end(lp))
					lp++;
				n = ANYSKIP;
				(*score)++;
				break;
			case CHR:
				c = *(ap+1);
				while (!end(lp) && c == re_tolower(*lp))
					lp++;
				n = CHRSKIP;
				(*score) += 100;
				break;
			case CCL:
				while ((c = re_tolower(*lp)) && isinset(ap+1,c))
					lp++;
				n = CCLSKIP;
				(*score) += 2;
				break;
			default:
				r->error = RE_ERR_CLOBADNFA;
				r->fail_op = *ap;
				return 0;
			}

			ap += n;

			while (lp >= are) {
				e = pmatch(r, lp, lpend, ap, score);
				if (e)
					return e;
				--lp;
			}
			return 0;
		default:
			r->error = RE_ERR_BADNFA;
			r->fail_op = op;
			return 0;
		}
	return lp;
}

/*
 * re_modw:
 *	add new characters into the word table to change re_exec's
 *	understanding of what a word should look like. Note that we
 *	only accept additions into the word definition.
 *
 *	If the string parameter is 0 or null string, the table is
 *	reset back to the default containing A-Z a-z 0-9 _. [We use
 *	the compact bitset representation for the default table]
 */

static CHAR deftab[16] = {
	0, 0, 0, 0, 0, 0, 0377, 003, 0376, 0377, 0377, 0207,
	0376, 0377, 0377, 007 
};

void RE_PRFX(modw)(RE_PRFX(t) *r, char *s)
{
	register int i;

	if (!s || !*s) {
		for (i = 0; i < MAXCHR; i++)
			if (!isinset(deftab,i))
				iswordc(i) = 0;
	}
	else
		while(*s)
			iswordc(*s++) = 1;
}


static void dappend(RE_PRFX(t) *r,char c)
{
	grow(r, dst, 1);
	r->dst[r->dst_used++] = c;
}

static void dappend_multi(RE_PRFX(t) *r, const char *s, int len)
{
	grow(r, dst, len);
	memcpy(r->dst + r->dst_used, s, len);
	r->dst_used += len;
}

/*
 * re_subs:
 *	substitute backreferences in src with the matched portions
 *	result goes in a dynamic allocated dst.
 *
 *	\digit	substitute a subpattern, with the given	tag number.
 *		Tags are numbered from 1 to 9. If the particular
 *		tagged subpattern does not exist, null is substituted.
 *
 *	\0	substitute the entire matched pattern.
 *
 * returns length of dst or -1 on error. dst points to a buffer thayt
 * is reused across subs() sessions.
 */
int RE_PRFX(backref)(RE_PRFX(t) *r, char **dst, const char *src SRCLEN)
{
	register char c;
	register int  pin;
	register const char *bp;
	register const char *ep;
#if RE_BIN_API == 1
	const char *src_end = src + srclen;
#endif


	if (dst != NULL) {
		*dst = 0;
		r->dst_used = 0;
	}

	if (!*src || !r->bopat[0])
		return 0;

#if RE_BIN_API == 1
	while (src < src_end) {
		c = *src++;
#else
	while ((c = *src++)) {
#endif
		switch(c) {

		case '\\':
			c = *src++;
			if (c >= '0' && c <= '9') {
				pin = c - '0';
				break;
			}
			
		default:
			dappend(r, c);
			continue;
		}

		if ((bp = r->bopat[pin]) && (ep = r->eopat[pin])) {
			while (*bp && bp < ep)
				dappend(r, *bp++);
			if (bp < ep)
				return -1;
		}
	}

	if (dst != NULL) {
#if RE_BIN_API != 1
		dappend(r, '\0');
#endif
		*dst = r->dst;
	}
	return r->dst_used;
}


int RE_PRFX(subst)(RE_PRFX(t) *r, char **dst, const char *src SRCLEN, const char *pat PATLEN, int first_only)
{
	const char *s, *s_end, *s_rest; /* src pointers */
	int cnt;

	s_rest = s = src;

#if RE_BIN_API == 1
	s_end = s + srclen;
#else
	for(s_end = s; *s_end != '\0'; s_end++) ;
#endif

	cnt = 0;
	r->dst_used = 0;

	for(;;) {
		/* find next match */
		if (RE_PRFX(exec)(r, s BIN_OR_EMPTY(s_end - s))) {
			cnt++;
			s_rest = r->eopat[0];
			/* append source until the match starts */
			dappend_multi(r, s, r->bopat[0] - s);
			/* append substituted part */
			RE_PRFX(backref)(r, NULL, pat PATLEN_);

			if (first_only)
				goto last;
			s = r->eopat[0];
		}
		else {
			/* no more matches, copy the rest of the string */
			goto last;
		}
	}


last:;
	/* append the rest */
		if (s_end - s_rest > 0)
			dappend_multi(r, s_rest, s_end - s_rest);
#if RE_BIN_API != 1
		dappend(r, '\0');
#endif
		*dst = r->dst;
		return r->dst_used;
}


void RE_PRFX(free)(RE_PRFX(t) *re)
{
	if (re->dst != NULL)
		free(re->dst);
	if (re->nfa != NULL)
		free(re->nfa);
	free(re);
}


re_error_t RE_PRFX(errno)(RE_PRFX(t) *re)
{
	return re->error;
}

int RE_PRFX(has_error)(RE_PRFX(t) *re)
{
	return re->error != 0;
}

#define setout(dest, val) \
	if ((dest) != NULL) \
		*(dest) = val;

int RE_PRFX(tag)(RE_PRFX(t) *re, int tagid, char **begin, char **end)
{
	if ((tagid < 0) || (tagid > MAXTAG)) {
		setout(begin, NULL);
		setout(end, NULL);
		return -1;
	}

	setout(begin, (char *)re->bopat[tagid]);
	setout(end, (char *)re->eopat[tagid]);
	return 0;
}

