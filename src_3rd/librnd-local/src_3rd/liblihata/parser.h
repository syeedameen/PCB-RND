#include "lihata.h"

/* states of the parser state machine - internal, not needed for ordinary uses */
typedef enum lht_pstate_e {
	LHT_ST_DEAD,        /* got an error or finished (valid eof) - any new byte will be ignored */
	LHT_ST_BODY,        /* body of a list, hash or table; also "body" of a file before the root node */
	LHT_ST_BACKSLASH,   /* next character is literal */
	LHT_ST_BSTRING,     /* brace string */
	LHT_ST_PREVALUE,    /* reading { before value starts */
	LHT_ST_TVALUE,      /* text value without braces until a sep */
	LHT_ST_COMMENT      /* reading comment */
} lht_pstate_t;

typedef struct lht_pstack_s  lht_pstack_t;

typedef enum lht_event_e {
	LHT_OPEN,            /* open event on node types li, ha and ta; ->value is NULL; ->name is never NULL (anonymous nodes are empty strings) */
	LHT_CLOSE,           /* close event on node types li, ha and ta; ->name and ->value are NULL, ->nt is invalid */
	LHT_TEXTDATA,        /* open+close+data on node types te and sy; ->value is the text value */
	LHT_COMMENT,         /* a single line of comment (out-of-band); ->name is the comment text, ->value is NULL, -> nt is invalid */
	LHT_EOF,             /* valid end-of-file; ->name is NULL, ->nt is invalid; received on an actual eof or most often earlier, when the root node is closed */
	LHT_ERROR            /* ->name is the error message, ->value is NULL, ->nt is invalid */
} lht_event_t;

typedef struct lht_parse_s lht_parse_t;
struct lht_parse_s {
	/* public read-write for the caller */
	void *user_data;  /* caller can attach data to the context; useful in callbacks */
	void (*event)(lht_parse_t *ctx, lht_event_t ev, lht_node_type_t nt, const char *name, const char *value);  /* called back by the parser */

	/* read-only public states */
	int line, col;

	/* private internal states */
	char *token;
	int tused, talloced;

	lht_pstack_t *stack;
	int sused, salloced;
};

/* Initialize parser context - leaves "public read-write for the caller" data
   untouched. */
void lht_parser_init(lht_parse_t *ctx);

/* Pass next character to the parser. Repeat until (including) the EOF, as long
   as LHTE_SUCCESS (0) is returned. If the parser finishes (root node is closed),
   LHTE_STOP is returned. On error one of the LHTE_* error codes is returned first,
   then LHTE_STOP on subsequent calls. */
lht_err_t lht_parser_char(lht_parse_t *ctx, int c);

/* Free all resources allocated during the parse (does not touch
   "public read-write for the caller" data. The context is left in
   an invalid state that most probably causes a crash if
   lht_parser_char() is called with it. It can be reused (even
   with the same user data) after a lht_parser_init() call. */
void lht_parser_uninit(lht_parse_t *ctx);


/* Internal */
lht_pstate_t lht_parser_get_state(lht_parse_t *ctx, int *explicit_type, int *explicit_name);

