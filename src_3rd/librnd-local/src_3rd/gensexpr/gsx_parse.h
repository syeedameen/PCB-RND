typedef enum gsx_parse_event_e {
	GSX_EV_OPEN,
	GSX_EV_CLOSE,
	GSX_EV_ATOM,
	GSX_EV_ERROR
} gsx_parse_event_t;

typedef struct gsx_parse_s gsx_parse_t;

struct gsx_parse_s {
	/* caller MUST fill in these fields before or after init: */
	void (*cb)(gsx_parse_t *ctx, gsx_parse_event_t ev, const char *data);
	void *user_ctx; /* filled in by the caller */
	char line_comment_char; /* if non-zero, lines starting with this char are comments till the newline */

	/* caller should not chaneg the rest of the fields */
	/* location */
	size_t offs, line, col, depth;

	/* internal states */
	char *atom;
	int used, alloced;
	unsigned char pstate; /* parser state */
	unsigned in_comment:1;
	unsigned last_newline:1;
	unsigned brace_quote:1; /* allow {} to behave like quotes */
};

typedef enum gsx_parse_res_e {
	GSX_RES_NEXT,  /* can insert next character of the stream */
	GSX_RES_ERROR, /* parse error, can't proceed, don't read more */
	GSX_RES_EOE    /* end of expression, don't read more */
} gsx_parse_res_t;

void gsx_parse_init(gsx_parse_t *ctx);
void gsx_parse_uninit(gsx_parse_t *ctx);
int gsx_parse_char(gsx_parse_t *ctx, int chr);
