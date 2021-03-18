#include <liblihata/dom.h>

/**** Style ****/
typedef struct {
	char *s;
	int used, alloced;
} lht_persbuf_t;

typedef enum {
	LHT_LOC_NAME_PRE,
	LHT_LOC_NAME_POST,
	LHT_LOC_VAL_PRE,
	LHT_LOC_VAL_IGNORE,
	LHT_LOC_VAL_POST,
	LHT_LOC_TERM,
	LHT_LOC_max
} lht_perslht_loc_t;

typedef enum {
	LHTPERS_ERR_ROOT_MISMATCH = 1024,
	LHTPERS_ERR_ROOT_MISSING,
	LHTPERS_ERR_POPPED_TOO_MUCH          /* broken input, unbalanced */
} lht_pers_err_t;

/* "persistent style" */
typedef struct lht_perstyle_s lht_perstyle_t;
struct lht_perstyle_s {
	/* style - also can be static output conf */
	lht_persbuf_t buff[LHT_LOC_max];  /* extra whitespace between tokens */
	unsigned has_eq:1;           /* there's an equation sign between name and value */
	unsigned val_brace:1;        /* value is braced */
	unsigned etype:1;            /* explicit type prefix even for te: (or li: in table) */
	unsigned ename:1;            /* explicit name */
	unsigned name_braced:1;      /* name is braced */

	/* volatile data */
	lht_perstyle_t *parent;

	lht_perslht_loc_t loc;

	lht_node_type_t type;
	char *name;
	char *text_data;

	/* misc */
	unsigned bumped_invalid_char:1;
	unsigned seen_closing_brc:1;
	unsigned composite_open:1;
	unsigned composite_close:1;

	unsigned pending_open:1; /* open of li, ha or ta without \n seen */
	unsigned pending_close_term:1;   /* closing brace seen, waiting for a \n or an opening brace; in case of an opening brace, run a finish like if we had a term */

	unsigned know_indent:1;          /* styled output: we have already calculated the value of need_indent */
	unsigned need_indent:1;          /* styled output: any of the buff[]s have a * in them */
	unsigned need_indent_name_pre:1;
	unsigned need_indent_val_pre:1;
	unsigned need_indent_val_post:1;
	unsigned need_indent_term:1;

	unsigned valid:1;  /* 1 if the style is complete */
};

/**** Output formatting table for new nodes ****/
typedef struct lhtpers_rule_s lhtpers_rule_t;

struct lhtpers_rule_s {
	const char **path;                 /* reverse-path match */
	const lht_perstyle_t *style;       /* styler to use for the current node */
	lhtpers_rule_t *hash_order;        /* if node is a hash, print children in this order (only first element of the hash should be considered) */
};

/**** User callbacks (events) ****/
typedef enum {
	LHTPERS_DISK,    /* keep the disk-value intact */
	LHTPERS_MEM,     /* use the value from the in-memory document */
	LHTPERS_INHIBIT  /* inhibit printing this node */
} lhtpers_ev_res_t;

typedef struct {
	/* Called for each text node */
	lhtpers_ev_res_t (*text)(void *ev_ctx, lht_perstyle_t *style, lht_node_t *inmem_node, const char *ondisk_value);

	/* Called for each child of a list that is empty in-memory but not empty
	   on-disk; default return: LHTPERS_INHIBIT (so new on-disk version matches
	   the empty list in-memory); LHTPERS_DISK copies/keeps the on-disk subtree */
	lhtpers_ev_res_t (*list_empty)(void *ev_ctx, lht_perstyle_t *style, lht_node_t *inmem_parent_node);

	/* Called for a list-child that is present on disk but is removed in-mem.
	   default return: LHTPERS_INHIBIT (so new on-disk version matches
	   the list in-memory); LHTPERS_DISK copies/keeps the on-disk child */
	lhtpers_ev_res_t (*list_elem_removed)(void *ev_ctx, lht_perstyle_t *style, lht_node_t *inmem_parent_node);

	void *ev_ctx;

	lhtpers_rule_t **output_rules; /* an array of rule table pointers */
	lht_perstyle_t *output_default_style[LHT_SYMLINK+1];
} lhtpers_ev_t;

/**** Persistent-format save ****/
lht_err_t lhtpers_save_as(const lhtpers_ev_t *events, lht_doc_t *doc, const char *infn, const char *outfn, char **errmsg);
lht_err_t lhtpers_fsave_as(const lhtpers_ev_t *events, lht_doc_t *doc, FILE *inf, FILE *outf, const char *infn, char **errmsg);


/**** style helper ****/
/* When this is the path of an ordered hash rule, the corresponding style's
   NAME_PRE buffer is printed in case no later rules ran from the list, else
   it is ignored. It is useful to inject a string upon early end-of-the-list. */
extern const char *lhtpers_early_end[];

/* Find the first rule that matches parent-path of subtree */
lhtpers_rule_t *lhtpers_rule_find(lhtpers_rule_t *table, lht_node_t *subtree);
