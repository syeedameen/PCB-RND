#ifndef LHT_DOM
#define LHT_DOM
#include <stdio.h>
#include "lihata.h"
#include "parser.h"
#include "genht/htsp.h"

typedef struct lht_doc_s lht_doc_t;
typedef struct lht_node_s lht_node_t;

typedef struct lht_text_s lht_text_t;
typedef struct lht_list_s lht_list_t;
typedef struct lht_hash_s lht_hash_t;
typedef struct lht_table_s lht_table_t;

struct lht_doc_s {
	/* -- public -- */
	lht_node_t *root;
	lht_node_t *unlinked; /* must be a LHT_LIST and holds unlinked subtrees */
	int detach_doc;       /* 1 if the doc was created by a node detach call (root is the detached node) */

	/* file names for location lookups - data could be a bool; data (pointer) points to doc instead */
	htsp_t *file_names;

	/* -- internal, for the dom parser -- */
	lht_node_t *active; /* last node seen on this level (level-determinator node) */
	lht_parse_t *p;
	const char *active_file; /* index of the file being parsed */
	int finished; /* status flag: 0 = parsing in progress; 1 = parsing is broken or finished */
	lht_err_t perror; /* parse error when finished != 0 */
	int err_col, err_line;  /* last line:col saved from the parser after an error (the parser struct is already free'd when this data is needed) */
};

struct lht_text_s {
	char *value;
};

struct lht_list_s {
	/* For easier appending to the end and inserting to the beginning of the list. */
	lht_node_t *first, *last;
};

struct lht_hash_s {
	htsp_t *tbl;
};

struct lht_table_s {
	int cols;           /* number of columns (number of items in row) */
	int cols_alloced;   /* size of each row array allocated (in number of nodes) */
	int rows;           /* number of rows (lists) in use */
	int rows_alloced;   /* size of the array (in rows) allocated */
	lht_node_t ***r; /* a row array of node ptr arrays */
	char **row_names;   /* we need to store row names, it's not guaranteed that they are anonymous */
};

struct lht_node_s {
	lht_node_type_t type;
	char *name;
	union {
		lht_text_t text;
		lht_list_t list;
		lht_hash_t hash;
		lht_table_t table;
		lht_text_t symlink;   /* symlink is just text that we interpret */
	} data;
	/* Used for storage within a single linked list only.
	   Can be NULL when not used (e.g. for single text node). */
	lht_node_t *next;

	/* tree: parent; children are stored in form of list/hash/table elements */
	lht_node_t *parent;
	lht_doc_t *doc;

	/* location */
	const char *file_name;
	int line, col;

	/* a lihata dom may be used as a generic data type, this is where the
	   user/caller may store its data */
	void *user_data;
};


lht_doc_t *lht_dom_init(void);
lht_err_t lht_dom_parser_char(lht_doc_t *doc, int c);
void lht_dom_uninit(lht_doc_t *doc);

/* Initialize a new document and load a file in root. The call will
   block until the file is loaded. Returns NULL on error and allocates
   and sets *errmsg (if errmsg != NULL).

   The stream version parses an already open FILE * and uses fn only
   for location info. */
lht_doc_t *lht_dom_load(const char *fn, char **errmsg);
lht_doc_t *lht_dom_load_stream(FILE *f, const char *fn, char **errmsg);
lht_doc_t *lht_dom_load_string(const char *str, const char *fn, char **errmsg);


/* --- loop iterators --- */
/* Order of iteration is specific to the parent node type:
    - list: in order from first to last
    - hash: in random order, each key only once
    - table: column first

   Iteration is done only on the first level (it's not a BFS or DFS).
   When there are no more children NULL is returned. Iterators
   don't allocate memory.

   WARNING: No write operation should be performed on parent while there are
   active iterations in progress!
*/

typedef struct lht_dom_iterator_s {
	/* read-only: */
	lht_node_t *parent;

	/* private: */
	union {
		lht_node_t *list_next;
		htsp_entry_t *hash_e;
		struct {
			int row, col;
		} tbl;
	} i;
} lht_dom_iterator_t;

/* returns the first child of parent and sets up the iterator (does not allocate it) */
lht_node_t *lht_dom_first(lht_dom_iterator_t *it, lht_node_t *parent);

/* returns the next child using an iterator set up by lht_dom_first() */
lht_node_t *lht_dom_next(lht_dom_iterator_t *it);


/* --- export --- */
typedef enum {
	LHT_STY_EQ_TE       = 0x0001,  /* put = in te: lines */
	LHT_STY_EQ_LI       = 0x0002,  /* put = in li: lines */
	LHT_STY_EQ_HA       = 0x0004,  /* put = in ha: lines */
	LHT_STY_EQ_TA       = 0x0008,  /* put = in ta: lines */
	LHT_STY_EQ_SY       = 0x0010,  /* put = in sy: lines */
	LHT_STY_EQ_TA_ROW   = 0x0020,  /* put = in ta: row lines */
	LHT_STY_BRACE_NAME  = 0x0100,  /* force bracing names even if no special chars in them */
	LHT_STY_BRACE_TE    = 0x0200,  /* force bracing te: value even if no special chars in value */
	LHT_STY_BRACE_SY    = 0x0400   /* force bracing sy: value even if no special chars in value */
} lht_dom_export_style_t;

/* prints a node recursively to outf, each line prefixed with prefix, in lihata format */
lht_err_t lht_dom_export_style(lht_node_t *node, FILE *outf, const char *prefix, lht_dom_export_style_t exp_style);
lht_err_t lht_dom_export(lht_node_t *node, FILE *outf, const char *prefix);

/* returns non-zero if txt needs {} around it while exporting */
int lht_need_brace(lht_node_type_t type, const char *txt, int is_name);


/* --- debug/diagnostic functions --- */

/* prints a node (no recursion) to outf, each line prefixed with prefix */
void lht_dom_pnode(lht_node_t *node, FILE *outf, const char *prefix);

/* prints a node recursively to outf, each line prefixed with prefix */
void lht_dom_ptree(lht_node_t *node, FILE *outf, const char *prefix);

/* --- location manipulation --- */

/* announces the start of a new file (from the next character passed to the parser);
   sets file name to name and line:col to 0:0
   returns whether the location was already known in this doc */
int lht_dom_loc_newfile(lht_doc_t *doc, const char *name);

/* return the current location of the parser (useful for error reporting); line and col are counted from 0 */
void lht_dom_loc_active(lht_doc_t *doc, const char **fn, int *line, int *col);

/* --- low level --- */

/* allocate a floating node of a given type. */
lht_node_t *lht_dom_node_alloc(lht_node_type_t type, const char *name);

/* duplicate a subtree into a newly allocated floating subtree */
lht_node_t *lht_dom_duptree(lht_node_t *src);

/* free node and its children recursively - does not unlink the node from the
   tree. WARNING: calling this without detaching/unlinking first may leave invalid
   pointers around; in most applications calling lht_tree_del() is a better
   choice. */
void lht_dom_node_free(lht_node_t *node);


/* --- list accessors --- */

/* append a detached node to the end of the list */
lht_err_t lht_dom_list_append(lht_node_t *dest, lht_node_t *node);

/* insert a detached node before the first element of the list */
lht_err_t lht_dom_list_insert(lht_node_t *dest, lht_node_t *node);

/* insert a detached node after dest (dest's parent must be a list) */
lht_err_t lht_dom_list_insert_after(lht_node_t *dest, lht_node_t *node);

/* returns the number of elements on the list */
int lht_dom_list_len(lht_node_t *list);

/* --- hash accessors --- */
/* insert detached node in the hash table */
lht_err_t lht_dom_hash_put(lht_node_t *hash, lht_node_t *node);

/* find the node by name (or return NULL) */
lht_node_t *lht_dom_hash_get(const lht_node_t *hash, const char *name);


/* --- table accessors --- */
/* returns the node located at row;col or NULL if out of bounds; col;row are counted from 0 */
lht_node_t *lht_dom_table_cell(lht_node_t *table, int row, int col);

/* Copies the content of src_nonempty to dst_empty; dst_empty must be an
   empty table created with lht_dom_node_alloc(LHT_TABLE, ...) */
int lht_dom_duptable(lht_node_t *dst_empty, lht_node_t *src_nonempty);

#endif
