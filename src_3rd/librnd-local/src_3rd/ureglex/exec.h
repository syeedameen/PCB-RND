#ifndef UREGLEX_EXEC_COMMON_H
#define UREGLEX_EXEC_COMMON_H

#define MAXTAG  10

typedef struct ureglex_precomp_s {
	const unsigned char *nfa;                 /* precompiled regex bytecode */
	const unsigned char *bittab;              /* bit table for CCL pre-set bits */
	const unsigned char *chrtyp;
	double weight;
} ureglex_precomp_t;

typedef struct ureglex_s {
	ureglex_precomp_t *pc;

	/* tags */
	const char *bol;
	const char *bopat[MAXTAG];
	const char *eopat[MAXTAG];

	int score;

	const char *endp;       /* last valid character in the input buffer + 1 */

	union { const void *ptr; int i; } pmstk[30];
	int pmsp;
	const unsigned char *pm_ap;
	const char *pm_lp;
	int pm_c;
	const char *pm_bp;      /* beginning of subpat.. */
	const char *pm_ep;      /* ending of subpat.. */
	const char *pm_are;     /* to save the line ptr. */

	const char *ex_lp;
	unsigned char ex_c;

	int ex_loop, pm_loop, pm_loop2, pm_loop2_later;

	int exec_state;
}  ureglex_t;

typedef enum {
	UREGLEX_MORE = -1,
	UREGLEX_TOO_LONG = -2, /* token too long, token buffer overrun */
	UREGLEX_NO_MATCH = -3, /* token matched no rule */
	UREGLEX_NOP = -4       /* token read and accepted but shall be ignored */
} ureglex_error_t;


extern const unsigned char ureglex_nfa_str[];

/* Used locally by used code */
#define ULX_BUF ctx->buff

#define ULX_TAGP(n) (ctx->state[ruleid].bopat[(n)])
#define ULX_TAGL(n) (ctx->state[ruleid].eopat[(n)] - ctx->state[ruleid].bopat[(n)])

#define ULX_IGNORE goto ureglex_ignore;

void ureglex_exec_init(ureglex_t *r, const char *lp, int buff_used);
int ureglex_exec(ureglex_t *r);

#endif
