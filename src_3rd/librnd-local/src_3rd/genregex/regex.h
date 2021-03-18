#ifndef GENREGEX_H
#define GENREGEX_H

typedef enum re_error_e {
	RE_ERR_SUCCESS = 0,

/* re_*_comp: */
	RE_ERR_NOPREV,
	RE_ERR_NOCLCL,
	RE_ERR_EMPTYCLO,
	RE_ERR_BADCLO,
	RE_ERR_STACK,
	RE_ERR_EMPTYTAG,
	RE_ERR_UNMATCHTAGC,
	RE_ERR_EMPTYWORD,
	RE_ERR_CYCREF,
	RE_ERR_UNTERMREF,
	RE_ERR_UNMATCHTAGO,
	RE_ERR_CLOBADNFA,
	RE_ERR_BADNFA
} re_error_t;

const char *re_error_str(re_error_t code);

#endif
