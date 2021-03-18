/*
    scconfig - dns API detection
    Copyright (C) 2010..2012  Tibor Palinkas

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

#include "socket.h"

int find_socket_getaddrinfo(const char *name, int logdepth, int fatal)
{
	const char *ldf;
	char *test_c =
		NL "#include <stdlib.h>"
		NL "#include <string.h>"
		NL "int main() {"
		NL "	struct addrinfo req, *ans;"
		NL "	memset(&req, 0, sizeof(req));"
		NL "	req.ai_flags    = 0;"
		NL "	req.ai_family   = PF_INET;"
		NL "	req.ai_socktype = SOCK_STREAM;"
		NL "	req.ai_protocol = IPPROTO_TCP;"
		NL "	SOCK_INIT;"
		NL "	if (getaddrinfo(\"127.0.0.1\", NULL, &req, &ans) != 0)"
		NL "		return 1;"
		NL "	freeaddrinfo(ans);"
		NL "	puts(\"OK\");"
		NL "	SOCK_UNINIT;"
		NL "	return 0;"
		NL "}"
		NL;

	char *inc_netdb =
		NL "#include <netdb.h>"
		NL;

	char *inc_netdb_in =
		NL "#include <netinet/in.h>"
		NL "#include <netdb.h>"
		NL;

	char *inc_win32 = "!"
		NL "#define _WIN32_WINNT 0x0501"
		NL;

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal)) /* for SOCK_INIT and implicit includes */
		return 1;

	report("Checking for getaddrinfo... ");
	logprintf(logdepth, "find_socket_getaddrinfo: trying to find getaddrinfo()...\n");
	logdepth++;

	ldf = get("libs/socket/socket/ldflags");

	/* Look at some standard places */
	if (try_socket(logdepth, "libs/socket/getaddrinfo", inc_netdb, test_c, NULL, ldf, 1)) return 0;
	if (try_socket(logdepth, "libs/socket/getaddrinfo", inc_netdb_in, test_c, NULL, ldf, 1)) return 0;
	if (try_socket(logdepth, "libs/socket/getaddrinfo", inc_win32, test_c, NULL, ldf, 1)) return 0;
	return 1;
}

int find_socket_getnameinfo(const char *name, int logdepth, int fatal)
{
	char *test_c =
		NL "#include <stdlib.h>"
		NL "#include <string.h>"
		NL "int main() {"
		NL "	struct sockaddr_in sa;"
		NL "	char *buf;"
		NL ""
		NL "	memset(&sa, 0, sizeof(sa));"
		NL "#ifdef SA_LEN"
		NL "		sa.SA_LEN        = sizeof(sa);"
		NL "#endif"
		NL "	sa.sin_family      = AF_INET;"
		NL "	sa.sin_addr.s_addr = 0x0100007f;"
		NL ""
		NL "	buf = malloc(1024);"
		NL "	SOCK_INIT;"
		NL "	if (getnameinfo((struct sockaddr *)&sa, sizeof(sa), buf, 1024, NULL, 0, 0))"
		NL "		return 1;"
		NL "	puts(\"OK\");"
		NL "	SOCK_UNINIT;"
		NL "	return 0;"
		NL "}"
		NL;

	char *inc_linux = NL "#include <netdb.h>";

	char *inc_bsd =
		NL "#include <netdb.h>"
		NL "#include <netinet/in.h>";

	char *def_bsd = "#define SA_LEN sin_len";

	char *inc_win32 = "!"
		NL "#define _WIN32_WINNT 0x0501"
		NL "#include <ws2tcpip.h>"
		NL;

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal)) /* for SOCK_INIT and implicit includes */
		return 1;

	report("Checking for getnameinfo... ");
	logprintf(logdepth, "find_socket_getnameinfo: trying to find getnameinfo()...\n");
	logdepth++;

	/* Look at some standard places */
	if (try_socket(logdepth, "libs/socket/getnameinfo", inc_linux, test_c, NULL, NULL, 1)) return 0;
	if (try_socket(logdepth, "libs/socket/getnameinfo", inc_bsd,   test_c, NULL, NULL, 1)) return 0;
	if (try_socket_def(logdepth, "libs/socket/getnameinfo", inc_bsd, def_bsd, test_c, NULL, NULL, 1)) {
		put("libs/socket/getnameinfo/sa_len", "sin_len");
		return 0;
	}
	if (try_socket(logdepth, "libs/socket/getnameinfo", inc_win32, test_c, NULL, "-lws2_32", 1)) return 0;
	return 1;
}


int find_socket_gethostname(const char *name, int logdepth, int fatal)
{
	const char *ldf;
	char *test_c =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	char buf[1024];"
		NL "	if (gethostname(buf, sizeof(buf)) == 0)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	char *inc_linux =
		NL "#include <netdb.h>";

	char *inc_unix =
		NL "#include <unistd.h>";

	char *inc_win1 =
		NL "#include <winsock2.h>";

	/* winsock2.h is usually lowercase */
	char *inc_win2 =
		NL "#include <Winsock2.h>";

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal))
		return 1;

	report("Checking for gethostname... ");
	logprintf(logdepth, "find_socket_gethostname: trying to find gethostname()...\n");
	logdepth++;

	ldf = get("libs/socket/socket/ldflags");

	/* Look at some standard places */
	if (try_socket(logdepth, "libs/socket/gethostname", inc_linux, test_c, NULL, ldf, 1)) return 0;
	if (try_socket(logdepth, "libs/socket/gethostname", inc_unix, test_c, NULL, ldf, 1)) return 0;
	if (try_socket(logdepth, "libs/socket/gethostname", inc_win1, test_c, NULL, ldf, 1)) return 0;
	if (try_socket(logdepth, "libs/socket/gethostname", inc_win2, test_c, NULL, ldf, 1)) return 0;
	return 1;
}

