#ifndef LHT_H
#define LHT_H

typedef enum lht_node_type_e {
	LHT_INVALID_TYPE = 0,
	LHT_TEXT,
	LHT_LIST,
	LHT_HASH,
	LHT_TABLE,
	LHT_SYMLINK
} lht_node_type_t;

/* returns the textual name or 2-char ID of a lihata type or NULL on error;
   LHT_INVALID_TYPE is not an error. */
const char *lht_type_name(lht_node_type_t type);
const char *lht_type_id(lht_node_type_t type);

/* The reverse lookup; slow, linear search */
lht_node_type_t lht_type_name2enum(const char *name);
lht_node_type_t lht_type_id2enum(const char *name);


/* parser constants common to both parsers */
/* parser errors */
typedef enum lht_err_e {
	LHTE_STOP = -1,
	LHTE_SUCCESS = 0,
	LHTE_STACK_UNDERFLOW,
	LHTE_INV_STATE_BSTRING,
	LHTE_INV_STATE_TVALUE,
	LHTE_NUL_CHAR,
	LHTE_EOF_BACKSLASH,
	LHTE_EOF_BSTRING,
	LHTE_EOF_NODE,
	LHTE_EOF_VALUE,
	LHTE_EOF_TVALUE,
	LHTE_INVALID_TYPE,
	LHTE_COLON,
	LHTE_ESCALHTE_NONTEXT,
	LHTE_INVALID_CHAR_PREVALUE,
	LHTE_INV_TEXT,
	LHTE_NO_BODY,
	LHTE_INTERNAL_PARSER,
	LHTE_TABLE_ROW_TYPE,
	LHTE_TABLE_COLS,
	LHTE_DUPLICATE_KEY,
	LHTE_NOT_IN_DOC,
	LHTE_BROKEN_DOC,
	LHTE_PATH_NOT_FOUND,
	LHTE_PATH_UNKNOWN_CWD,
	LHTE_PATH_CHILD_OF_TEXT,
	LHTE_PATH_INT,
	LHTE_PATH_INVALID_NODE,
	LHTE_PATH_SYMLINK_TOO_DEEP,
	LHTE_PATH_UNDERSPEC_TABLE,
	LHTE_NOT_FOUND,
	LHTE_BOUNDARY,
	LHTE_MERGE_TYPE_MISMATCH,
	LHTE_MERGE_SYMLINK_MISMATCH,
	LHTE_MERGE_EMPTY_SYMLINK,
	LHTE_MERGE_TABLECOLS_MISMATCH,
	LHTE_MERGE_CYCLIC,
	LHTE_OUT_OF_MEM,
	LHTE_WOULD_RESULT_INVALID_DOC,
	LHTE_NOT_DETACHED
} lht_err_t;

/* returns textual description of a parser error */
const char *lht_err_str(lht_err_t pe);

/* Safe (NULL results NULL), portable (C89) strdup; segfaults if malloc() fails. */
char *lht_strdup(const char *s);

#endif
