/*
 * The following defines are not meant to be changeable.
 * They are for readability only.
 */
#define MAXCHR	128
#define CHRBIT	8
#define BITBLK	MAXCHR/CHRBIT
#define BLKIND	0170
#define BITIND	07

void ureglex_exec_init(ureglex_t *re, const char *str, int buff_used);
extern int ureglex_exec(ureglex_t *re);
extern int ureglex_tag(ureglex_t *re, int tagid, char **begin, char **end);

enum ureglex_opcode_e {
	NOP = 0,
	END = 0,

	CHR = 1,   /* specific char         */
	ANY = 2,   /* any character:   .    */
	CCL = 3,   /* character class: []   */
	BOL = 4,   /* begin-of-line:   ^    */
	EOL = 5,   /* end-of-line:     $    */
	BOT = 6,   /* begin-of-tag:    \(   */
	EOT = 7,   /* end-of-tag:      \)   */
	BOW = 8,   /* begin-of-word:   \<   */
	EOW = 9,   /* end-of-word:     \>   */
	REF = 10,  /* backref:         \1   */
	CLO = 11   /* closure               */
};

