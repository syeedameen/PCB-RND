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

#include <string.h>
#include <assert.h>
#include "exec.h"
#include "common.h"

#define re_tolower(c) c
#define iswordc(r, x) r->pc->chrtyp[inascii(x)]

#define end(s)   (*(s) == '\0')

static unsigned char ureglex_bitarr[] = {1,2,4,8,16,32,64,128};

#define inascii(x)    (0177&(x))
#define isinset(x,y)  ((x)[((y)&BLKIND)>>3] & ureglex_bitarr[(y)&BITIND])

const unsigned char ureglex_nfa_str[] = {0};

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
 * skip values for CLO XXX to skip past the closure
 */

#define ANYSKIP 2   /* [CLO] ANY END ...         */
#define CHRSKIP 3   /* [CLO] CHR chr END ...     */
#define CCLSKIP 18  /* [CLO] CCL 16bytes END ... */

static const char MORE[] = "more!";

#define want_more(loopid) \
do { \
	if (r->pm_lp < r->endp) \
		goto loop ## loopid; \
	r->pm_loop = loopid; \
	return MORE; \
} while(0)

#define PUSH(r, ty, val) if (r->pmsp >= sizeof(r->pmstk) / sizeof(r->pmstk[0])) return 0; r->pmstk[r->pmsp++].ty = val
#define POP(r, ty, dst)  dst = r->pmstk[--r->pmsp].ty

static const char *pmatch(ureglex_t *r)
{
	register int op, c, n;
	register const char *e = NULL;       /* extra pointer for CLO */

	switch(r->pm_loop) {
		case 1: r->pm_loop = 0; goto loop1;
		case 2: r->pm_loop = 0; goto loop2;
		case 3: r->pm_loop = 0; goto loop3;
		case 4: r->pm_loop = 0; goto loop4;
/*		case 5: r->pm_loop = 0; goto loop5;*/
		case 6: r->pm_loop = 0; goto loop6;
		case 7: r->pm_loop = 0; goto loop7;
		case 8: r->pm_loop = 0; goto loop8;
	}

	switch(r->pm_loop2) {
		case 1: r->pm_loop2 = 0; goto loop2_1;
	}

	while ((op = *r->pm_ap++) != END)
		switch(op) {

		case CHR:
			want_more(6);
			loop6:;
			if (re_tolower(*r->pm_lp++) != *r->pm_ap++)
				return 0;
			r->score += 100;
			break;
		case ANY:
			want_more(7);
			loop7:;
			if (end(r->pm_lp++))
				return 0;
			r->score++;
			break;
		case CCL:
			if (end(r->pm_lp))
				return 0;
			want_more(8);
			loop8:;
			c = re_tolower(*r->pm_lp++);
			if (!isinset(r->pm_ap,c))
				return 0;
			r->pm_ap += BITBLK;
			r->score += 2;
			break;
		case BOL:
			if (r->pm_lp != r->bol)
				return 0;
			r->score += 10;
			break;
		case EOL:
			if (!end(r->pm_lp))
				return 0;
			r->score += 10;
			break;
		case BOT:
			r->bopat[*r->pm_ap++] = r->pm_lp;
			break;
		case EOT:
			r->eopat[*r->pm_ap++] = r->pm_lp;
			break;
		case BOW:
			if ((r->pm_lp!=r->bol && iswordc(r, r->pm_lp[-1])) || !iswordc(r, *r->pm_lp))
				return 0;
			r->score += 5;
			break;
		case EOW:
			if (r->pm_lp==r->bol || !iswordc(r, r->pm_lp[-1]) || (!end(r->pm_lp) && iswordc(r, *r->pm_lp)))
				return 0;
			r->score += 5;
			break;
		case REF:
			n = *r->pm_ap++;
			r->pm_bp = r->bopat[n];
			r->pm_ep = r->eopat[n];
			while (r->pm_bp < r->pm_ep) {
				want_more(1);
				loop1:;
				if (*r->pm_bp++ != *r->pm_lp++)
					return 0;
				r->score += 2;
			}
			break;
		case CLO:
			r->pm_are = r->pm_lp;
			switch(*r->pm_ap) {

			case ANY:
				do {
					want_more(2);
					loop2:;
				} while(!end(r->pm_lp++));
				n = ANYSKIP;
				r->score++;
				break;
			case CHR:
				r->pm_c = *(r->pm_ap+1);
				do {
					want_more(3);
					loop3:;
				} while (!end(r->pm_lp) && r->pm_c == re_tolower(*r->pm_lp) && (r->pm_lp++));
				n = CHRSKIP;
				r->score += 100;
				break;
			case CCL:
				do {
					want_more(4);
					loop4:;
				} while ((c = re_tolower(*r->pm_lp)) && isinset(r->pm_ap+1,c) && (r->pm_lp++));
				n = CCLSKIP;
				r->score += 2;
				break;
			default:
				return 0;
			}

			r->pm_ap += n;

			while (r->pm_lp >= r->pm_are) {
				PUSH(r, ptr, r->pm_ap);
				PUSH(r, ptr, r->pm_lp);
				PUSH(r, i, r->pm_loop);
				r->pm_loop2_later = 1; /* continue on loop2_1 once the recursion "fully" exited  (it will return multiple times for MORE) */
				e = pmatch(r);
				if (e == MORE)
					return MORE; /* r->pm_loop was set up in the recursion */
				loop2_1:;
				POP(r, i, r->pm_loop);
				POP(r, ptr, r->pm_lp);
				POP(r, ptr, r->pm_ap);
				if (e)
					return e;
				--r->pm_lp;
			}
			return 0;
		default:
			return 0;
		}
	r->pm_loop2 = r->pm_loop2_later; /* we may be a recursion, pmatch called pmatch; in this case this is how we get the next call to set back the "stack" at loop2_1 */
	return r->pm_lp;
}

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
void ureglex_exec_init(ureglex_t *r, const char *lp, int buff_used)
{
	r->bol = lp;
	r->score = 1;

	memset(r->bopat, 0, (char *)&r->eopat[MAXTAG] - (char *)&r->bopat[0]);

	r->pmsp = 0;

	r->ex_lp = lp;
	r->endp = lp + buff_used;

	r->ex_loop = r->pm_loop = r->pm_loop2 = 0;

	r->exec_state = -1;
}

#undef want_more

#define want_more(loopid) \
do { \
	if (r->ex_lp < r->endp) \
		goto loop ## loopid; \
	r->ex_loop = loopid; \
	return -1; \
} while(0)

#define want_more2(loopid) \
do { \
	if (r->pm_lp < r->endp) \
		goto loop ## loopid; \
	r->ex_loop = loopid; \
	return -1; \
} while(0)

int ureglex_exec(ureglex_t *r)
{
	register const char *ep = 0;
	const unsigned char *ap = r->pc->nfa;

	r->endp++;

	switch(r->ex_loop) {
		case 1: r->ex_loop = 0; goto loop1;
		case 2: r->ex_loop = 0; goto loop2;
		case 3: r->ex_loop = 0; goto loop3;
		case 4: r->ex_loop = 0; goto loop4;
	}

	switch(*ap) {

	case BOL:                 /* anchored: match from BOL only */
		r->pm_ap = ap;
		r->pm_lp = r->ex_lp;
		loop1:;
		ep = pmatch(r);
		if (ep == MORE)
			want_more2(1);
		break;
	case CHR:                 /* ordinary char: locate it fast */
		r->ex_c = *(ap+1);
		while (!end(r->ex_lp) && re_tolower(*r->ex_lp) != r->ex_c) {
			r->ex_lp++;
			want_more(2);
			loop2:;
		}
		if (end(r->ex_lp))      /* if EOS, fail */
			return 0;
		/* fall thru */
	default:                  /* regular matching all the way. */
		for(;;) {
			r->pm_ap = ap;
			r->pm_lp = r->ex_lp;

			loop3:;
			ep = pmatch(r);
			if (ep == MORE) {
				want_more2(3);
			}

			if (ep != NULL)
				break;
			r->ex_lp++;
			want_more(4);
			loop4:;
			if (end(r->ex_lp))
				break;
		}
		break;
	case END:                 /* munged automaton. fail always */
		return 0;
	}
	if (!ep)
		return 0;

	r->bopat[0] = r->ex_lp;
	r->eopat[0] = ep;
	return r->score;
}

#define setout(dest, val) \
	if ((dest) != NULL) \
		*(dest) = val;

int ureglex_tag(ureglex_t *re, int tagid, char **begin, char **end)
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

