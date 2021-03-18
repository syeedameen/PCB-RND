/*
    scconfig - socket API detection
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

#include "socket.h"

int find_socket_select(const char *name, int logdepth, int fatal)
{
	char *test_c =
		NL "#include <stdlib.h>"
		NL "int main() {"
		NL "	struct timeval tv;"
		NL "  tv.tv_sec = 0;"
		NL "  tv.tv_usec = 1;"
		NL "	SOCK_INIT;"
		NL "	select(0, NULL, NULL, NULL, &tv);"
		NL "	puts(\"OK\");"
		NL "	SOCK_UNINIT;"
		NL "	return 0;"
		NL "}"
		NL;

	char *inc_linux =
		NL "#include <sys/select.h>"
		NL;

	char *inc_old =
		NL "#include <sys/time.h>"
		NL "#include <sys/types.h>"
		NL "#include <unistd.h>"
		NL;

	char *inc_win = NL "#include <winsock2.h>" NL;

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal)) /* for SOCK_INIT and implicit includes */
		return 1;

	report("Checking for select... ");
	logprintf(logdepth, "find_socket_select: trying to find select()...\n");
	logdepth++;

	/* Look at some standard places */
	if (try_socket(logdepth, "libs/socket/select", inc_linux, test_c, NULL, NULL, 1)) return 0;
	if (try_socket(logdepth, "libs/socket/select", inc_old, test_c, NULL, NULL, 1)) return 0;
	if (try_socket(logdepth, "libs/socket/select", inc_win, test_c, NULL, "-lws2_32", 1)) return 0;

	report("not found\n");
	return 1;
}

int find_socket_poll(const char *name, int logdepth, int fatal)
{
	char *test_c =
		NL "#include <stdlib.h>"
		NL "int main() {"
		NL "	struct pollfd fds[1];"
		NL "	fds[0].fd = 0;"
		NL "	fds[0].events = POLLIN;"
		NL "	SOCK_INIT;"
		NL "	if (poll(fds, 1, 1) == 0)"
		NL "	puts(\"OK\");"
		NL "	SOCK_UNINIT;"
		NL "	return 0;"
		NL "}"
		NL;

	char *inc_linux =
		NL "#include <poll.h>"
		NL;

	if (require("cc/cc", logdepth, fatal))
		return 1;
	if (require("libs/socket/socket/presents", logdepth, fatal)) /* for SOCK_INIT and implicit includes */
		return 1;

	report("Checking for poll... ");
	logprintf(logdepth, "find_socket_poll: trying to find poll()...\n");
	logdepth++;

	/* Look at some standard places */
	if (try_socket(logdepth, "libs/socket/poll", inc_linux, test_c, NULL, NULL, 1)) return 0;

	report("not found\n");
	return 1;
}
