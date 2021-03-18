/*
    scconfig - socket API detection
    Copyright (C) 2010..2012  Tibor Palinkas
    Copyright (C) 2018  Aron Barath

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
#include <time.h>

int find_socket_socket(const char *name, int logdepth, int fatal)
{
	char *test_c =
		NL "int main() {"
		NL "	int s;"
		NL "	SOCK_INIT;"
		NL "	s = socket(AF_INET, SOCK_STREAM, 0);"
		NL "	if (s >= 0)"
		NL "		puts(\"OK\");"
		NL "	SOCK_UNINIT;"
		NL "	return 0;"
		NL "}"
		NL;

	char *inc_linux =
		NL "#include <sys/types.h>"
		NL "#include <sys/socket.h>"
		NL;

	char *inc_windows =
		NL "#include <winsock2.h>"
		NL "#include <ws2tcpip.h>"
		NL "#include <windows.h>"
		NL;

	char *def_empty =
		NL "#define SOCK_INIT"
		NL "#define SOCK_UNINIT"
		NL;

#define DEF_WINDOWS_INIT "do { WSADATA w; WSAStartup(MAKEWORD(2,2), &w); } while(0)"
#define DEF_WINDOWS_UNINIT "WSACleanup()"
	char *def_windows =
		NL "#define SOCK_INIT " DEF_WINDOWS_INIT
		NL "#define SOCK_UNINIT " DEF_WINDOWS_UNINIT
		NL;

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for socket... ");
	logprintf(logdepth, "find_socket_socket: trying to find socket()...\n");
	logdepth++;

	/* normally init code is empty */
	put("libs/socket/socket/init_code",      "");
	put("libs/socket/socket/uninit_code",    "");
	put("libs/socket/socket/type",           "int");
	put("libs/socket/socket/invalid_socket", "-1");

	/* Look at some standard places */
	/* on most UNIX systems */
	if (try_socket_pure(logdepth, "libs/socket/socket", inc_linux, def_empty, test_c, NULL, NULL, 1)) return 0;

	/* this combination seems to be present on sysV forks */
	if (try_socket_pure(logdepth, "libs/socket/socket", inc_linux, def_empty, test_c, NULL, "-lsocket -lnsl", 1)) return 0;

	/* Windows has its own socket library (try with WSA init) */
	if (try_socket_pure(logdepth, "libs/socket/socket", inc_windows, def_windows, test_c, NULL, "-lws2_32", 1)) {
		put("libs/socket/socket/init_code",      DEF_WINDOWS_INIT);
		put("libs/socket/socket/uninit_code",    DEF_WINDOWS_UNINIT);
		put("libs/socket/socket/type",           "SOCKET");
		put("libs/socket/socket/invalid_socket", "INVALID_SOCKET");
		return 0;
	}

	/* final fallback: it may be that a sysV-like system has either -lsocket or -lnsl but not both */
	if (try_socket_pure(logdepth, "libs/socket/socket", inc_linux, def_empty, test_c, NULL, "-lsocket", 1)) return 0;
	if (try_socket_pure(logdepth, "libs/socket/socket", inc_linux, def_empty, test_c, NULL, "-lnsl", 1)) return 0;

	put("libs/socket/socket/presents", sfalse);
	report("not found\n");

	return 1;
}

int find_socket_wsasocket(const char *name, int logdepth, int fatal)
{
	char *test_c =
		NL "int main() {"
		NL "	int s;"
		NL "	SOCK_INIT;"
		NL "	s = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);"
		NL "	if (s >= 0)"
		NL "		puts(\"OK\");"
		NL "	SOCK_UNINIT;"
		NL "	return 0;"
		NL "}"
		NL;

	char *inc_windows =
		NL "#include <windows.h>"
		NL "#include <winsock2.h>"
		NL "#include <ws2tcpip.h>"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for WSASocket... ");
	logprintf(logdepth, "find_socket_wsasocket: trying to find socket()...\n");
	logdepth++;

	/* WSASocket should be specific to win32 socket lib */
	if (try_socket(logdepth, "libs/socket/wsasocket", inc_windows, test_c, NULL, "-lws2_32", 1)) return 0;

	put("libs/socket/wsasocket/presents", sfalse);
	report("not found\n");

	return 1;
}

char *test_c_socketpair =
		NL "#include <stdio.h>"
		NL "#include <unistd.h>"
		NL "int main()"
		NL "{"
		NL "	int fd[2];"
		NL "	int ret, len;"
		NL "	char s[16];"
		NL ""
		NL "	SOCK_INIT;"
		NL "	ret = socketpair(%s, %s, 0, fd);"
		NL "	if (ret != 0)"
		NL "		return 1;"
		NL "	write(fd[0], \"test1\\n\", 5);"
		NL "	len = read(fd[1], s, sizeof(s));"
		NL "	if (len > 0)"
		NL "	puts(\"OK\");"
		NL "	SOCK_UNINIT;"
		NL "	return 0;"
		NL "}"
		NL;


int find_socket_socketpair(const char *name, int logdepth, int fatal)
{
	char *test_c;
	const char **family;
	const char *families[] = {"AF_LOCAL", "PF_UNIX", NULL};

	char *inc_linux =
		NL "#include <sys/types.h>"
		NL "#include <sys/socket.h>"
		NL;


	if (require("cc/cc", logdepth, fatal))
		return 1;

	if (require("libs/socket/socket/presents", logdepth, fatal)) /* for SOCK_INIT and implicit includes */
		return 1;

	report("Checking for socketpair()... ");
	logprintf(logdepth, "find_socket_socketpair: trying to find socketpair()...\n");
	logdepth++;

	test_c = malloc(strlen(test_c_socketpair)+32);
	for(family = families; *family != NULL; family++) {
		sprintf(test_c, test_c_socketpair,  *family, "SOCK_STREAM");

		/* Look at some standard places */
		if (try_socket(logdepth, "libs/socket/socketpair", inc_linux, test_c, NULL, NULL, 1)) {
			put("libs/socket/socketpair/af_local", *family);
			free(test_c);
			return 0;
		}
	}
	free(test_c);

	put("libs/socket/socketpair/presents", sfalse);
	report("not found\n");
	return 1;
}

int find_socket_types(const char *name, int logdepth, int fatal)
{
	char node[256];
	struct {
		char *domain;
		char *type;
	} *t, type_combos[] = {
		{"AF_UNIX",  "SOCK_STREAM"},
		{"AF_UNIX",  "SOCK_DGRAM"},
		{"AF_UNIX",  "SOCK_SEQPACKET"},
		{"AF_INET",  "SOCK_STREAM"},
		{"AF_INET",  "SOCK_DGRAM"},
		{"AF_INET",  "SOCK_SEQPACKET"},
		{"AF_INET6", "SOCK_STREAM"},
		{"AF_INET6", "SOCK_DGRAM"},
		{"AF_INET6", "SOCK_SEQPACKET"},
		{NULL, NULL}
	};
	char *test_c_socket =
		NL "int main() {"
		NL "	SOCK_INIT;"
		NL "	if (socket(%s, %s, 0) >= 0) {"
		NL "		puts(\"OK\");"
		NL "		return 0;"
		NL "	}"
		NL "	SOCK_UNINIT;"
		NL "	return 1;"
		NL "}"
		NL;
	char *test_c;

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal)) /* for SOCK_INIT and implicit includes */
		return 1;

	report("Checking for socket domain/type combinations... ");

	test_c = malloc(strlen(test_c_socket)+32);
	for(t = type_combos; t->domain != NULL; t++) {
		sprintf(test_c, test_c_socket, t->domain, t->type);
		sprintf(node, "libs/socket/types/%s/%s", t->domain, t->type);
		try_socket(logdepth, node, "", test_c, NULL, NULL, 0);
	}
	report("done\n");

	free(test_c);
	put("libs/socket/types", strue);
	return 0;
}


int find_socket_ioctlsocket(const char *name, int logdepth, int fatal)
{
	char test_c_temp[] =
		NL "int main() {"
		NL "	int s;"
		NL "	%s nonblocking;"
		NL "	SOCK_INIT;"
		NL "	s = socket(AF_INET, SOCK_STREAM, 0);"
		NL "	ioctl%s(s, 0, &nonblocking);"
		NL "	puts(\"OK\");"
		NL "	SOCK_UNINIT;"
		NL "	return 0;"
		NL "}"
		NL;
	char test_c[sizeof(test_c_temp)+64];
	const char *ldf;

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal)) /* for SOCK_INIT and implicit includes */
		return 1;

	report("Checking for ioctlsocket... ");
	logprintf(logdepth, "find_socket_ioctlsocket: trying to find ioctlsocket()...\n");
	logdepth++;

	sprintf(test_c, test_c_temp, "int", "");
	if (try_socket(logdepth, "libs/socket/ioctl", "", test_c, NULL, NULL, 0)) {
		put("libs/socket/ioctl/ioctl", "ioctl");
		put("libs/socket/ioctl/presents", strue);
		put("libs/socket/ioctl/argtype", "int");
		put("libs/socket/ioctlsocket/presents", sfalse);
		report("OK (ioctl, arg type int)\n");
		return 0;
	}

	sprintf(test_c, test_c_temp, "u_long", "socket");
	ldf = get("libs/socket/socket/ldflags");
	if (try_socket(logdepth, "libs/socket/ioctl", "", test_c, NULL, ldf, 0)) {
		put("libs/socket/ioctlsocket/presents", strue);
		put("libs/socket/ioctl/argtype", "u_long");
		put("libs/socket/ioctl/presents", sfalse);
		put("libs/socket/ioctl/ioctl", "ioctlsocket");
		report("OK (ioctlsocket, arg type u_long)\n");
		return 0;
	}

	put("libs/socket/ioctlsockets/presents", sfalse);
	report("ioctl for sockets not found\n");
	return 1;
}

int find_socket_fionbio(const char *name, int logdepth, int fatal)
{
	const char *ldf;
	char test_cf[] =
		NL "int main() {"
		NL "	int s;"
		NL "	%s nonblocking = 1;"
		NL "	SOCK_INIT;"
		NL "	s = socket(AF_INET, SOCK_STREAM, 0);"
		NL "	%s(s, FIONBIO, &nonblocking);"
		NL "	puts(\"OK\");"
		NL "	SOCK_UNINIT;"
		NL "	return 0;"
		NL "}"
		NL;
	char test_c[256];

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal)) /* for SOCK_INIT and implicit includes */
		return 1;
	if (require("libs/socket/ioctl/ioctl", logdepth, fatal))
		return 1;


	report("Checking for socket FIONBIO... ");
	logprintf(logdepth, "find_socket_fionbio: trying to find fionbio()...\n");
	logdepth++;

	sprintf(test_c, test_cf, get("libs/socket/ioctl/argtype"), get("libs/socket/ioctl/ioctl"));
	ldf = get("libs/socket/socket/ldflags");
	if (try_socket(logdepth, "libs/socket/ioctl/fionbio", "", test_c, NULL, ldf, 0)) {
		put("libs/socket/ioctl/fionbio/presents", strue);
		report("found (win32-style)\n");
		return 0;
	}

	sprintf(test_c, test_cf, get("libs/socket/ioctl/argtype"), get("libs/socket/ioctl/ioctl"));
	if (try_socket(logdepth, "libs/socket/ioctl/fionbio", "#include <sys/ioctl.h>", test_c, NULL, NULL, 0)) {
		put("libs/socket/ioctl/fionbio/presents", strue);
		report("found (UNIX-style)\n");
		return 0;
	}

	sprintf(test_c, test_cf, get("libs/socket/ioctl/argtype"), get("libs/socket/ioctl/ioctl"));
	if (try_socket(logdepth, "libs/socket/ioctl/fionbio", "#define BSD_COMP\\n#include <sys/ioctl.h>", test_c, NULL, NULL, 0)) {
		put("libs/socket/ioctl/fionbio/presents", strue);
		report("found (UNIX-style with BSD_COMP)\n");
		return 0;
	}

	put("libs/socket/ioctl/fionbio/presents", sfalse);
	report("not found\n");
	return 1;
}

int find_socket_closesocket(const char *name, int logdepth, int fatal)
{
	char test_c[] =
		NL "int main() {"
		NL "	int s;"
		NL "	SOCK_INIT;"
		NL "	s = socket(AF_INET, SOCK_STREAM, 0);"
		NL "	closesocket(s);"
		NL "	puts(\"OK\");"
		NL "	SOCK_UNINIT;"
		NL "	return 0;"
		NL "}"
		NL;

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal))
		return 1;

	report("Checking for closesocket... ");
	logprintf(logdepth, "find_socket_closesocket: trying to find closesocket()...\n");
	logdepth++;

	if (try_socket(logdepth, "libs/socket/closesocket", "", test_c, NULL, NULL, 0)) {
		put("libs/socket/closesocket/presents", strue);
		report("found\n");
		return 0;
	}

	put("libs/socket/closesocket/presents", sfalse);
	report("not found\n");
	return 1;
}

int find_socket_SHUT(const char *name, int logdepth, int fatal)
{
	char test_c[] =
		NL "int main() {"
		NL "	int i;"
		NL "	i = SHUT_WR + SHUT_RD + SHUT_RDWR;"
		NL "	puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;


	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal))
		return 1;


	report("Checking for SHUT_* constants... ");
	logprintf(logdepth, "find_socket_SHUT: trying to find SHUT_* constants...\n");
	logdepth++;

	if (try_socket(logdepth, "libs/socket/SHUT", "", test_c, NULL, NULL, 0)) {
		report("found\n");
		return 0;
	}

	put("libs/socket/SHUT/presents", sfalse);
	report("not found\n");
	return 1;
}

int find_socket_SD(const char *name, int logdepth, int fatal)
{
	char test_c[] =
		NL "int main() {"
		NL "	int i;"
		NL "	i = SD_RECEIVE + SD_SEND + SD_BOTH;"
		NL "	puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal))
		return 1;

	report("Checking for SD_* constants... ");
	logprintf(logdepth, "find_socket_SD: trying to find SD_* constants...\n");
	logdepth++;

	if (try_socket(logdepth, "libs/socket/SD", "", test_c, NULL, NULL, 0)) {
		report("found\n");
		return 0;
	}

	put("libs/socket/SD/presents", sfalse);
	report("not found\n");
	return 1;
}

int find_socket_shutdown(const char *name, int logdepth, int fatal)
{
	const char *ldf;
	char test_c[] =
		NL "int main() {"
		NL "	int s;"
		NL "	SOCK_INIT;"
		NL "	s = socket(AF_INET, SOCK_STREAM, 0);"
		NL "	shutdown(s, 0);"
		NL "	puts(\"OK\");"
		NL "	SOCK_UNINIT;"
		NL "	return 0;"
		NL "}"
		NL;

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal))
		return 1;


	report("Checking for shutdown... ");
	logprintf(logdepth, "find_socket_shutdown: trying to find shutdown()...\n");
	logdepth++;

	ldf = get("libs/socket/socket/ldflags");

	if (try_socket(logdepth, "libs/socket/shutdown", "", test_c, NULL, ldf, 0)) {
		report("found\n");
		return 0;
	}

	put("libs/socket/shutdown/presents", sfalse);
	report("not found\n");
	return 1;
}

int find_socket_sockaddr_in(const char *name, int logdepth, int fatal)
{
	const char *ldf;
	static char *test_c =
		NL "#include <stdlib.h>"
		NL "#include <stdio.h>"
		NL "#include <string.h>"
		NL "int main() {"
		NL "	struct sockaddr_in sa;"
		NL "	char buff[64];"
		NL "	int ls, cs, as;"
		NL ""
		NL "	SOCK_INIT;"
		NL "	ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);"
		NL "	cs = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);"
		NL ""
		NL "	memset(&sa, 0, sizeof(sa));"
		NL "	sa.sin_family      = AF_INET;"
		NL "	sa.sin_addr.s_addr = 0x0100007f;"
		NL "	sa.sin_port = 4242;"
		NL "	printf(\"OK\\n\");"
		NL "	return 0;"
		NL "}"
		NL;

	char *inc_db =
		NL "#include <netdb.h>"
		NL;

	char *inc_inet =
		NL "#include <netinet/in.h>"
		NL;

	if (require("cc/cc", logdepth, fatal))
		return 1;

	report("Checking for sockaddr_in... ");
	logprintf(logdepth, "find_socket_sockaddr_in: trying to find sockaddr_in...\n");
	logdepth++;


	ldf = get("libs/socket/socket/ldflags");
	/* Look at some standard places */
	if (try_socket(logdepth, "libs/socket/sockaddr_in", "", test_c, NULL, ldf, 1)) return 0;
	if (try_socket(logdepth, "libs/socket/sockaddr_in", inc_db, test_c, NULL, ldf, 1)) return 0;
	if (try_socket(logdepth, "libs/socket/sockaddr_in", inc_inet, test_c, NULL, ldf, 1)) return 0;

	put("libs/socket/sockaddr_in/presents", sfalse);
	report("not found\n");
	return 1;
}

static int test_lac_port = 48223;
static char *test_lac =
	NL "%s"
	NL "#include <stdlib.h>"
	NL "#include <stdio.h>"
	NL "#include <string.h>"
	NL "int main() {"
	NL "	struct sockaddr_in sa;"
	NL "	char buff[64];"
	NL "	int ls, cs, as, b, n;"
	NL ""
	NL "	SOCK_INIT;"
	NL "	ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);"
	NL "	cs = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);"
	NL ""
	NL "	memset(&sa, 0, sizeof(sa));"
	NL "	sa.sin_family      = AF_INET;"
	NL "	sa.sin_addr.s_addr =  %s;"
	NL "	sa.sin_port = %d;"
	NL "	for(n = 0; n < 32; n++) {"
	NL "		b = bind(ls, (struct sockaddr *)&sa, sizeof(sa));"
	NL "		if (b == 0)"
	NL "			break;"
	NL "	sa.sin_port += %d; /* if failed to bind prot, try another */"
	NL "	}"
	NL "		if (b != 0) /* failed to bind any port in many tries */"
	NL "			return 1;"
	NL "	fprintf(stderr, \"l=%%d ls=%%d\\n\", listen(ls, 2), ls);"
	NL "	fprintf(stderr, \"c=%%d cs=%%d\\n\", connect(cs, (struct sockaddr *)&sa, sizeof(sa)), cs);"
	NL "	as = accept(ls, NULL, NULL);"
	NL "	fprintf(stderr, \"as=%%d\\n\", as);"
	NL "%s"
	NL "	puts(\"OK\");"
	NL "	SOCK_UNINIT;"
	NL "	return 0;"
	NL "}"
	NL;

static const char *localhost(void)
{
	/* do not depend on any macro for localhost */
	if (strcmp(get("sys/byte_order"), "LSB") == 0)
		return "0x0100007f";
	return "0x7f000001";
}

int find_socket_lac(const char *name, int logdepth, int fatal)
{
	const char *ldf;
	char test_c[2048];
	int test_lac_port_inc;

	char *inc_linux =
		NL;

	char *inc_dns;

	if (require("cc/cc", logdepth, fatal))
		return 1;

	if (require("libs/socket/socket/presents", logdepth, fatal))
		return 1;

	if (require("libs/socket/sockaddr_in/presents", logdepth, fatal))
		return 1;

	if (require("sys/byte_order", logdepth, fatal))
		return 1;


	inc_dns = str_subsn(get("libs/socket/sockaddr_in/includes"));

	report("Checking for lac... ");
	logprintf(logdepth, "find_socket_lac: trying to find lac...\n");
	logdepth++;

	srand(time(NULL));
	test_lac_port ^= (rand() % 10000);
	if (test_lac_port > 65500)
		test_lac_port -= 65500;
	if (test_lac_port < 1024)
		test_lac_port += 1024;


	test_lac_port_inc = (rand() % 32);

	sprintf(test_c, test_lac, inc_dns, localhost(), test_lac_port++, test_lac_port_inc, "");
	free(inc_dns);

	ldf = get("libs/socket/socket/ldflags");

	/* Look at some standard places */
	if (try_socket(logdepth, "libs/socket/lac", inc_linux, test_c, NULL, ldf, 1)) return 0;

	put("libs/socket/lac/presents", sfalse);
	report("not found\n");
	return 1;
}

int find_socket_recvsend(const char *name, int logdepth, int fatal)
{
	const char *ldf;
	int test_lac_port_inc;
	char test_c[2048];

	char *test_local =
		NL "*buff = '\\0';"
		NL "send(cs, \"test\", 4, 0);"
		NL "recv(as, buff, 4, 0);"
		NL "buff[4] = '\\0';"
		NL "fprintf(stderr, \"buff='%s' (should be 'test')\\n\", buff);"
		NL "if (strcmp(buff, \"test\") != 0)"
		NL "	return 1;"
		NL ;

	char *inc_linux =
		NL;

	char *inc_dns;

	if (require("cc/cc", logdepth, fatal))
		return 1;

	if (require("libs/socket/socket/presents", logdepth, fatal))
		return 1;

	if (require("libs/socket/sockaddr_in/presents", logdepth, fatal))
		return 1;

	if (require("sys/byte_order", logdepth, fatal))
		return 1;

	inc_dns = str_subsn(get("libs/socket/sockaddr_in/includes"));

	report("Checking for recv() and send()... ");
	logprintf(logdepth, "find_socket_recvsend: trying to find recv() and send()...\n");
	logdepth++;

	test_lac_port_inc = (rand() % 32);

	sprintf(test_c, test_lac, inc_dns, localhost(), test_lac_port++, test_lac_port_inc, test_local);
	free(inc_dns);

	ldf = get("libs/socket/socket/ldflags");

	/* Look at some standard places */
	if (try_socket(logdepth, "libs/socket/recvsend", inc_linux, test_c, NULL, ldf, 1)) return 0;

	put("libs/socket/recvsend/presents", sfalse);
	report("not found\n");
	return 1;
}

int find_socket_readwrite(const char *name, int logdepth, int fatal)
{
	char test_c[2048];
	int test_lac_port_inc;

	char *test_local =
		NL "*buff = '\\0';"
		NL "write(cs, \"test\", 4);"
		NL "read(as, buff, 4);"
		NL "buff[4] = '\\0';"
		NL "fprintf(stderr, \"buff='%s' (should be 'test')\\n\", buff);"
		NL "if (strcmp(buff, \"test\") != 0)"
		NL "	return 1;"
		NL ;

	char *inc_linux =
		NL;

	char *inc_dns;

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal))
		return 1;
	if (require("libs/socket/sockaddr_in/presents", logdepth, fatal))
		return 1;
	if (require("sys/byte_order", logdepth, fatal))
		return 1;

	inc_dns = str_subsn(get("libs/socket/sockaddr_in/includes"));

	report("Checking for read() and write()... ");
	logprintf(logdepth, "find_socket_recvsend: trying to find read() and write()...\n");
	logdepth++;

	test_lac_port_inc = (rand() % 32);

	sprintf(test_c, test_lac, inc_dns, localhost(), test_lac_port++, test_lac_port_inc, test_local);
	free(inc_dns);

	/* Look at some standard places */
	if (try_socket(logdepth, "libs/socket/readwrite", inc_linux, test_c, NULL, NULL, 1)) return 0;

	put("libs/socket/readwrite/presents", sfalse);
	report("not found\n");
	return 1;
}

int find_socket_ntoh(const char *name, int logdepth, int fatal)
{
	char test_c[] =
		NL "int main() {"
		NL "	short int sh;"
		NL "	long int ln;"
		NL "	sh = ntohs(htons(42));"
		NL "	ln = ntohl(htonl(42));"
		NL "	if ((sh == 42) && (ln == 42))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	char *inc[] = {"#include <arpa/inet.h>", "#include <winsock2.h>", NULL};
	char **i;

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal))
		return 1;


	report("Checking for ntoh macros... ");
	logprintf(logdepth, "find_socket_ntoh: trying to find ntoh macros...\n");
	logdepth++;

	for(i = inc; *i != NULL; i++) {
		if (try_socket_(logdepth, *i, test_c, NULL, NULL)) {
			put("libs/socket/ntoh/presents", strue);
			put("libs/socket/ntoh/includes", *i);
			report("OK (%s)\n", *i);
			return 0;
		}

		if (try_socket_(logdepth, *i, test_c, NULL, "-lws2_32")) {
			put("libs/socket/ntoh/presents", strue);
			put("libs/socket/ntoh/includes", *i);
			report("OK (%s)\n", *i);
			return 0;
		}
	}

	put("libs/socket/ntoh/presents", sfalse);
	report("not found\n");
	return 1;
}

int find_socket_inetaddr(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	SOCK_INIT;"
		NL "	if (inet_addr(\"1.2.3.4\") != INADDR_NONE)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	if (require("cc/cc", logdepth, fatal))
		return 1;

	if (require("libs/socket/socket/presents", logdepth, fatal))
		return 1;

	report("Checking for inet_addr()... ");
	logprintf(logdepth, "find_socket_inetaddr: trying to find inet_addr...\n");
	logdepth++;

	if (try_socket(logdepth, "libs/socket/inet_addr", "#include <sys/socket.h>\n#include <netinet/in.h>\n#include <arpa/inet.h>\n", test_c, NULL, NULL, 1)) return 0;
	if (try_socket(logdepth, "libs/socket/inet_addr", "#include <winsock2.h>\n#include <windows.h>\n", test_c, NULL, NULL, 1)) return 0;
	return try_fail(logdepth, "libs/socket/inet_addr");
}

int find_socket_gethostbyname(const char *name, int logdepth, int fatal)
{
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	SOCK_INIT;"
		NL "	if (gethostbyname(\"127.0.0.1\"))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	if (require("cc/cc", logdepth, fatal))
		return 1;

	if (require("libs/socket/socket/presents", logdepth, fatal))
		return 1;

	report("Checking for gethostbyname()... ");
	logprintf(logdepth, "find_socket_gethostbyname: trying to find gethostbyname()...\n");
	logdepth++;

	if (try_socket(logdepth, "libs/socket/gethostbyname", "#include <netdb.h>\n", test_c, NULL, NULL, 1)) return 0;
	if (try_socket(logdepth, "libs/socket/gethostbyname", "#include <winsock2.h>\n#include <ws2tcpip.h>\n#include <windows.h>\n", test_c, NULL, NULL, 1)) return 0;
	return try_fail(logdepth, "libs/socket/gethostbyname");
}

#if 0
int find_socket_auxtypes(const char *name, int logdepth, int fatal)
{
	const char *test_c_template =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	%s x = 1;"
		NL "	if (x)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	char test_c[1024];
	char node[128];
	char *nodeend;
	const char *includes[] = {
		"#include <sys/socket.h>",
		"#include <netinet/in.h>",
		"#include <winsock2.h>\n#include <windows.h>\n",
		"#include <winsock2.h>\n#include <ws2tcpip.h>\n#include <windows.h>\n",
		NULL};
	const char *types[] = {"socklen_t", "in_port_t", "in_addr_t", "sa_family_t", NULL};
	const char **inc;
	const char **typ;

	if (require("cc/cc", logdepth, fatal))
		return 1;

	strcpy(node, "libs/socket/");
	nodeend = node + strlen(node);

	report("Checking for auxiliary socket-related types... ");
	logprintf(logdepth, "find_socket_auxtypes: trying to find auxiliary socket-related types...\n");
	logdepth++;

	for (typ=types; *typ; ++typ) {
		sprintf(test_c, test_c_template, *typ);
		strcpy(nodeend, *typ);
		for (inc=includes; *inc; ++inc) {
			if (try_icl(logdepth, node, test_c, *inc, NULL, NULL))
				break;
		}
	}

	return 0;
}
#endif

static int find_socket_auxtype_impl(const char *name, int logdepth, int fatal, const char *typ)
{
	const char *test_c_template =
		NL "#include <stdio.h>"
		NL "int main() {"
		NL "	%s x = 1;"
		NL "	if (x)"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	char test_c[1024];
	char node[128];
	const char *includes[] = {
		"#include <sys/socket.h>",
		"#include <netinet/in.h>",
		"#include <winsock2.h>\n#include <windows.h>\n",
		"#include <winsock2.h>\n#include <ws2tcpip.h>\n#include <windows.h>\n",
		NULL};
	const char **inc;

	if (require("cc/cc", logdepth, fatal))
		return 1;

	sprintf(node, "libs/socket/%s", typ);

	report("Checking for type '%s'... ", typ);
	logprintf(logdepth, "find_socket_auxtypes: trying to find type '%s'...\n", typ);
	logdepth++;

	sprintf(test_c, test_c_template, typ);
	for (inc=includes; *inc; ++inc) {
		if (try_icl(logdepth, node, test_c, *inc, NULL, NULL)) {
			report(" found ('%s')\n", *inc);
			return 0;
		}
	}

	report(" not found\n");
	return 1;
}

int find_socket_inaddrt(const char *name, int logdepth, int fatal)
{
	return find_socket_auxtype_impl(name, logdepth, fatal, "in_addr_t");
}

int find_socket_inportt(const char *name, int logdepth, int fatal)
{
	return find_socket_auxtype_impl(name, logdepth, fatal, "in_port_t");
}

int find_socket_socklent(const char *name, int logdepth, int fatal)
{
	return find_socket_auxtype_impl(name, logdepth, fatal, "socklen_t");
}

int find_socket_safamilyt(const char *name, int logdepth, int fatal)
{
	return find_socket_auxtype_impl(name, logdepth, fatal, "sa_family_t");
}
