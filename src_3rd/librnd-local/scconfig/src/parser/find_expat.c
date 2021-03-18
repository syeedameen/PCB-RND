/*
    scconfig - libexpat detection
    Copyright (C) 2010  Tibor Palinkas

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

#include "parser.h"
#include <unistd.h>


int find_parser_expat(const char *name, int logdepth, int fatal)
{
	char *test_c =
		NL "int main() {"
		NL "	void *parser;"
		NL "	parser = XML_ParserCreate(NULL);"
		NL "	if (parser != NULL)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for expat... ");
	logprintf(logdepth, "find_parser_expat: trying to find libexpat...\n");
	logdepth++;

	/* Look at some standard places */
	if (try_icl(logdepth, "libs/parser/expat", test_c, "#include <expat.h>", NULL, "-lexpat")) return 0;
	return try_fail(logdepth, "libs/parser/expat");
}
