#ifndef UREGLEX_STRTREE_H
#define UREGLEX_STRTREE_H
typedef enum {ULX_REQ = 1, ULX_BRA, ULX_FIN, ULX_BAD} ureglex_stree_op_t;

/*
	REQ = next is a char literal
	BRA = next is char literal then branch-on-macth addr
	FIN = next is the rule number finished
	BAD = no match possible, ignore until reset (next token)
*/

typedef struct ureglex_strtree_s { int *code, *ip; } ureglex_strtree_t;

int ureglex_strtree_exec(ureglex_strtree_t *ctx, int chr);

#define UREGLEX_STRTREE_MORE -5


#endif
