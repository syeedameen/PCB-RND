#include "strtree.h"

int ureglex_strtree_exec(ureglex_strtree_t *ctx, int chr)
{
	int expected, dst;
	for(;;) {
/*printf("[%d] ", ctx->ip - ctx->code);*/
		switch(*ctx->ip) {
			case ULX_REQ:
				expected = ctx->ip[1];
				ctx->ip += 2;
				if (chr == expected)
					return UREGLEX_STRTREE_MORE;
				return -1; /* bad */
			case ULX_BRA:
				expected = ctx->ip[1];
				dst = ctx->ip[2];
				ctx->ip += 3;
				if (chr == expected) {
					ctx->ip = ctx->code + dst;
					return UREGLEX_STRTREE_MORE;
				}
				break; /* next BRA or BAD */
			case ULX_FIN: return ctx->ip[1];
			case ULX_BAD: return -2;
		}
	}
	return -1;
}
