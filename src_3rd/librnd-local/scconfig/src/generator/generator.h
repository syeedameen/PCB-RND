int generate(const char *input, const char *output);

typedef enum {
	mode_copy,
	mode_cmd,
	mode_copy_sep,
	mode_cmd_sep,
	mode_fatal
} genmode_t;

char *generator_eval(const char *input, genmode_t mode);

void generator_emit_char(int c);
void generator_emit_string(const char *s);
void generator_emit_separators(int count);
