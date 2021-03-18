/*
    scconfig - c99 feature detection
    Copyright (C) 2009  Szabolcs Nagy
    Copyright (C) 2017  Aron Barath

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

		Project page: http://repo.hu/projects/scconfig
		Contact via email: scconfig [at] igor2.repo.hu
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libs.h"
#include "log.h"
#include "db.h"
#include "dep.h"

/*
NOTES. please read before extending this file.
The process of adding a new test is:
	1. copy one of the c99_find_*() functions
	2. replace the test program and/or the code that is looking for alternatives
	3. add your new c99_find_*() in deps_c99_init(); make sure deps_c99_init() is in the bottom of the file
	4. edit ../../doc/tree.txt, add your new detection under std/c99/
	5. to test recompile in trunk/ and run ./configure --detect=std/c99/YOUR_NODE_NAME
*/

static int try(int logdepth, const char *cc, const char *test_c, const char *cflags, const char *ldflags, const char *expected)
{
	char *out;

	logprintf(logdepth, "trying cc99:try with cc='%s' cflags='%s' ldflags='%s'\n", (cc == NULL ? get("cc/cc") : cc), cflags == NULL ? "" : cflags, ldflags == NULL ? "" : ldflags);

	if (compile_run(logdepth+1, test_c, cc, cflags, ldflags, &out) == 0) {
		if (((out == NULL) && (iscross)) || (strncmp(out, expected, strlen(expected)) == 0)) {
			free(out);
			return 1;
		}
		free(out);
	}
	return 0;
}

int find_c99_stdc(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "#if __STDC_VERSION__ >= 199901L"
		NL "	puts(\"OK\");"
		NL "#endif"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 __STDC_VERSION__ macro... ");
	logprintf(logdepth, "find_c99_stdc: trying to find c99 __STDC_VERSION__ macro...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "OK")) {
		put("std/c99/stdc", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/stdc", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_emptymacro(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "#define d(x,y,z) x # y # z"
		NL "int main() {"
		NL "	puts(d(,,OK));"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 empty macro... ");
	logprintf(logdepth, "find_c99_emptymacro: trying to find c99 empty macro...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "OK")) {
		put("std/c99/emptymacro", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/emptymacro", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_variadicmacro(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "#define p(...) printf(__VA_ARGS__)"
		NL "int main() {"
		NL "	p(\"%s\\n\", \"OK\");"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 variadic macro... ");
	logprintf(logdepth, "find_c99_variadicmacro: trying to find c99 variadic macro...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "OK")) {
		put("std/c99/variadicmacro", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/variadicmacro", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_funcmacro(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "void OK() {puts(__func__);}"
		NL "int main() {"
		NL "	OK();"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 __func__ macro... ");
	logprintf(logdepth, "find_c99_funcmacro: trying to find c99 __func__ macro...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "OK")) {
		put("std/c99/funcmacro", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/funcmacro", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_stdint(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		/* <stdint.h>:  [u]intNN_t, [u]int_fastNN_t, [u]int_least_t, [u]intmax_t, [u]intptr_t, +limits */
		NL "#include <inttypes.h>" /* includes stdint +defines printf formats */
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	uint8_t a = 1;"
		NL "	int16_t b = 1;"
		NL "	int32_t c = 1;"
		NL "	int64_t d = 1;"
		NL "	printf(\"%\"PRIu8 \"%\"PRId16 \"%\"PRId32 \"%\"PRId64 \"\\n\", a, b, c, d);"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 stdint... ");
	logprintf(logdepth, "find_c99_stdint: trying to find c99 stdint...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "1111")) {
		put("std/c99/stdint", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/stdint", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_wchar(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stddef.h>"
		NL "wchar_t w[] = L\"OK\";"
		NL "#include <wchar.h>"
		NL "#include <wctype.h>"
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	printf(\"%ls\\n\", w);"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 wchar_t... ");
	logprintf(logdepth, "find_c99_wchar: trying to find c99 wchar_t...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "OK")) {
		put("std/c99/wchar", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/wchar", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_longlong(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	unsigned long long int a = 42ULL;"
		NL "	printf(\"%lld\\n\", a);"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 long long int... ");
	logprintf(logdepth, "find_c99_longlong: trying to find c99 long long int...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "42")) {
		put("std/c99/longlong", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/longlong", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_bool(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "#include <stdbool.h>"
		NL "int main() {"
		NL "	_Bool a = true;"
		NL "	bool b = false;"
		NL "	if (a && !b) puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 bool... ");
	logprintf(logdepth, "find_c99_bool: trying to find c99 bool...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "OK")) {
		put("std/c99/bool", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/bool", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_inline(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "inline int f() {puts(\"OK\");}"
		NL "int main() {"
		NL "	f();"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 inline function specifier... ");
	logprintf(logdepth, "find_c99_inline: trying to find c99 inline function specifier...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "OK")) {
		put("std/c99/inline", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/inline", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_restrict(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "void f(char * restrict s) {puts(s);}"
		NL "int main() {"
		NL "	f(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 restrict pointer qualifier... ");
	logprintf(logdepth, "find_c99_restrict: trying to find c99 restrict pointer qualifier...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "OK")) {
		put("std/c99/restrict", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/restrict", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_comment(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	// puts(\"comment\");"
		NL "	puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 // style comment... ");
	logprintf(logdepth, "find_c99_comment: trying to find c99 // style comment...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "OK")) {
		put("std/c99/comment", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/comment", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_endcomma(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "enum {A=12,B=30,};"
		NL "int main() {"
		NL "	int a[] = {A+B,};"
		NL "	printf(\"%d\\n\", a[0]);"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 end comma... ");
	logprintf(logdepth, "find_c99_endcomma: trying to find c99 end comma...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "42")) {
		put("std/c99/endcomma", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/endcomma", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_nonconstinit(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int f() {return 42;}"
		NL "int main() {"
		NL "	int a[] = {f()};"
		NL "	printf(\"%d\\n\", a[0]);"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 non-const initializer... ");
	logprintf(logdepth, "find_c99_nonconstinit: trying to find c99 non-const initializer...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "42")) {
		put("std/c99/nonconstinit", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/nonconstinit", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_init(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "struct s_t {int x; int y;};"
		NL "int main() {"
		NL "	int a[] = {[3]=10,[2]=12};"
		NL "	struct s_t s = {.x=20};"
		NL "	printf(\"%d\\n\", a[1]+a[2]+a[3]+s.x+s.y);"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 designated initializer... ");
	logprintf(logdepth, "find_c99_init: trying to find c99 designated initializer...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "42")) {
		put("std/c99/init", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/init", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_compoundlit(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int f(int *a) {return a[2];}"
		NL "int main() {"
		NL "	printf(\"%d\\n\", f((int[]){0,1,42}));"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 compound literals... ");
	logprintf(logdepth, "find_c99_compoundlit: trying to find c99 compound literals...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "42")) {
		put("std/c99/compoundlit", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/compoundlit", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_decl(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	int a;"
		NL "	a = 40;"
		NL "	int b;"
		NL "	b = 2;"
		NL "	printf(\"%d\\n\", a+b);"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 declaration and statement mixing... ");
	logprintf(logdepth, "find_c99_decl: trying to find c99 declaration and statement mixing...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "42")) {
		put("std/c99/decl", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/decl", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_for(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	for (int i = 0; i < 1; i++)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 for loop... ");
	logprintf(logdepth, "find_c99_for: trying to find c99 for loop...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "OK")) {
		put("std/c99/for", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/for", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_divmod(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	if(-22/7 == -3 && -22%7 == -1) puts(\"Fortran\");"
		NL "	if(-22/7 == -4 && -22%7 ==  6) puts(\"Pascal\");"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 negative div,mod... ");
	logprintf(logdepth, "find_c99_divmod: trying to find c99 negative div,mod...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "Fortran")) {
		put("std/c99/divmod", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/divmod", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_incomparray(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stddef.h>"
		NL "#include <stdio.h>"
		NL "struct t {int n; char s[];};"
		NL "int main() {"
		NL "	if(sizeof(struct t) == offsetof(struct t, s))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 incomplete array member... ");
	logprintf(logdepth, "find_c99_incomparray: trying to find c99 incomplete array member...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "OK")) {
		put("std/c99/incomparray", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/incomparray", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_snprintf(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	char s[4];"
		NL "	if(snprintf(s, 4, \"12345\") == 5)"
		NL "		puts(s);"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 snprintf... ");
	logprintf(logdepth, "find_c99_snprintf: trying to find c99 snprintf...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "123")) {
		put("std/c99/snprintf", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/snprintf", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_floatfmt(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	double f = 1.0;"
		NL "	long double Lf = 1.0;"
		NL "	printf(\"%.1F %.1lf %.1LF %.1a %.1A\\n\", f, f, Lf, f, f);"
		NL "	return 0;"
		NL "}"
		NL ;
	require("cc/cc", logdepth, fatal);

	report("Checking for c99 float printf formats... ");
	logprintf(logdepth, "find_c99_floatfmt: trying to find c99 float printf formats...\n");
	logdepth++;
	if (try(logdepth, NULL, test_c, NULL, NULL, "1.0 1.0 1.0 0x1.0p+0 0X1.0P+0")) {
		put("std/c99/floatfmt", strue);
		report("Found.\n");
		return 0;
	}
	put("std/c99/floatfmt", sfalse);
	report("Not found.\n");
	return 1;
}

int find_c99_strtomax(const char *det_name, int logdepth, int fatal)
{
	const char *test_c_template =
		NL "void my_puts(const char *s);"
		NL "typedef %s my_char;" /* "char" or "wchar_t" */
		NL "const my_char src_str[] = %s\"12345\";" /* "" or "L" */
		NL "int main()"
		NL "{"
		NL "	my_char *end = 0;"
		NL "	if (%s(src_str, &end, 0) == 12345 &&" /* {str,wcs}to{i,u}max */
		NL "		end &&"
		NL "		!*end)"
		NL "		my_puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL "#include <stdio.h>"
		NL "void my_puts(const char *s)"
		NL "{"
		NL "	puts(s);"
		NL "}"
		NL ;
	char test_c[1024];
	char key[128];
	char det_target[16];
	const char *includes[] = {
		"#include <inttypes.h>", /* strto{i,u}max */
		"#include <stddef.h>\n#include <inttypes.h>", /* wcsto{i,u}max */
		NULL};
	const char **inc;
	int i;
	require("cc/cc", logdepth, fatal);

	for (i=0; i<9; ++i)
		det_target[i] = det_name[8+i];
	det_target[i] = 0;

	report("Checking for c99 %s... ", det_target);
	logprintf(logdepth, "find_c99_strtomax: trying to find c99 %s...\n", det_target);
	logdepth++;

	if(det_target[0]=='s')
		sprintf(test_c, test_c_template, "char", "", det_target);
	else
		sprintf(test_c, test_c_template, "wchar_t", "L", det_target);

	sprintf(key, "std/c99/%s", det_target);

	for (inc=includes; *inc; ++inc)
		if (try_icl(logdepth, key, test_c, *inc, NULL, NULL)) {
			report("Found.\n");
			return 0;
		}

	report("Not found.\n");
	return 1;
}

/* TODO: the detected include is broken */

int find_c99_intptr_t(const char *name, int logdepth, int fatal)
{
	return find_types_something_t(name, logdepth, fatal, "std/c99", "intptr_t", NULL, "#include <stdint.h>");
}

int find_c99_uintptr_t(const char *name, int logdepth, int fatal)
{
	return find_types_something_t(name, logdepth, fatal, "std/c99", "uintptr_t", NULL, "#include <stdint.h>");
}

int find_c99_intmax_t(const char *name, int logdepth, int fatal)
{
	return find_types_something_t(name, logdepth, fatal, "std/c99", "intmax_t", NULL, "#include <stdint.h>");
}

int find_c99_uintmax_t(const char *name, int logdepth, int fatal)
{
	return find_types_something_t(name, logdepth, fatal, "std/c99", "uintmax_t", NULL, "#include <stdint.h>");
}

int find_c99_int8_t(const char *name, int logdepth, int fatal)
{
	return find_types_something_t(name, logdepth, fatal, "std/c99", "int8_t", NULL, "#include <stdint.h>");
}

int find_c99_uint8_t(const char *name, int logdepth, int fatal)
{
	return find_types_something_t(name, logdepth, fatal, "std/c99", "uint8_t", NULL, "#include <stdint.h>");
}

int find_c99_int16_t(const char *name, int logdepth, int fatal)
{
	return find_types_something_t(name, logdepth, fatal, "std/c99", "int16_t", NULL, "#include <stdint.h>");
}

int find_c99_uint16_t(const char *name, int logdepth, int fatal)
{
	return find_types_something_t(name, logdepth, fatal, "std/c99", "uint16_t", NULL, "#include <stdint.h>");
}

int find_c99_int32_t(const char *name, int logdepth, int fatal)
{
	return find_types_something_t(name, logdepth, fatal, "std/c99", "int32_t", NULL, "#include <stdint.h>");
}

int find_c99_uint32_t(const char *name, int logdepth, int fatal)
{
	return find_types_something_t(name, logdepth, fatal, "std/c99", "uint32_t", NULL, "#include <stdint.h>");
}

int find_c99_int64_t(const char *name, int logdepth, int fatal)
{
	return find_types_something_t(name, logdepth, fatal, "std/c99", "int64_t", NULL, "#include <stdint.h>");
}

int find_c99_uint64_t(const char *name, int logdepth, int fatal)
{
	return find_types_something_t(name, logdepth, fatal, "std/c99", "uint64_t", NULL, "#include <stdint.h>");
}


void deps_c99_init()
{
	dep_add("std/c99/stdc",           find_c99_stdc);
	dep_add("std/c99/emptymacro",     find_c99_emptymacro);
	dep_add("std/c99/variadicmacro",  find_c99_variadicmacro);
	dep_add("std/c99/funcmacro",      find_c99_funcmacro);
	dep_add("std/c99/stdint",         find_c99_stdint);
	dep_add("std/c99/wchar",          find_c99_wchar);
	dep_add("std/c99/longlong",       find_c99_longlong);
	dep_add("std/c99/bool",           find_c99_bool);
	dep_add("std/c99/inline",         find_c99_inline);
	dep_add("std/c99/restrict",       find_c99_restrict);
	dep_add("std/c99/comment",        find_c99_comment);
	dep_add("std/c99/endcomma",       find_c99_endcomma);
	dep_add("std/c99/nonconstinit",   find_c99_nonconstinit);
	dep_add("std/c99/init",           find_c99_init);
	dep_add("std/c99/compoundlit",    find_c99_compoundlit);
	dep_add("std/c99/decl",           find_c99_decl);
	dep_add("std/c99/for",            find_c99_for);
	dep_add("std/c99/divmod",         find_c99_divmod);
	dep_add("std/c99/incomparray",    find_c99_incomparray);
	dep_add("std/c99/snprintf",       find_c99_snprintf);
	dep_add("std/c99/floatfmt",       find_c99_floatfmt);
	dep_add("std/c99/int8_t/*",       find_c99_int8_t);
	dep_add("std/c99/uint8_t/*",      find_c99_uint8_t);
	dep_add("std/c99/int16_t/*",      find_c99_int16_t);
	dep_add("std/c99/uint16_t/*",     find_c99_uint16_t);
	dep_add("std/c99/int32_t/*",      find_c99_int32_t);
	dep_add("std/c99/uint32_t/*",     find_c99_uint32_t);
	dep_add("std/c99/int64_t/*",      find_c99_int64_t);
	dep_add("std/c99/uint64_t/*",     find_c99_uint64_t);
	dep_add("std/c99/intptr_t/*",     find_c99_intptr_t);
	dep_add("std/c99/uintptr_t/*",    find_c99_uintptr_t);
	dep_add("std/c99/intmax_t/*",     find_c99_intmax_t);
	dep_add("std/c99/uintmax_t/*",    find_c99_uintmax_t);
	dep_add("std/c99/strtoimax/*",    find_c99_strtomax);
	dep_add("std/c99/strtoumax/*",    find_c99_strtomax);
	dep_add("std/c99/wcstoimax/*",    find_c99_strtomax);
	dep_add("std/c99/wcstoumax/*",    find_c99_strtomax);
}
