/*
    scconfig - helper functions for detecting sockets
    Copyright (C) 2009..2012  Tibor Palinkas

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
#include <assert.h>
#include "find.h"
#include "regex.h"

void deps_socket_init()
{
	dep_add("libs/socket/socket/*",              find_socket_socket);
	dep_add("libs/socket/wsasocket/*",           find_socket_wsasocket);
	dep_add("libs/socket/socketpair/*",          find_socket_socketpair);
	dep_add("libs/socket/select/*",              find_socket_select);
	dep_add("libs/socket/poll/*",                find_socket_poll);
	dep_add("libs/socket/types",                 find_socket_types);
	dep_add("libs/socket/ioctlsocket/*",         find_socket_ioctlsocket);
	dep_add("libs/socket/ioctl/*",               find_socket_ioctlsocket);
	dep_add("libs/socket/ioctl/ioctl",           find_socket_ioctlsocket);
	dep_add("libs/socket/closesocket/*",         find_socket_closesocket);
	dep_add("libs/socket/shutdown/*",            find_socket_shutdown);
	dep_add("libs/socket/SHUT/*",                find_socket_SHUT);
	dep_add("libs/socket/SD/*",                  find_socket_SD);
	dep_add("libs/socket/ioctl/fionbio/*",       find_socket_fionbio);
	dep_add("libs/socket/sockaddr_in/*",         find_socket_sockaddr_in);
	dep_add("libs/socket/lac/*",                 find_socket_lac);
	dep_add("libs/socket/recvsend/*",            find_socket_recvsend);
	dep_add("libs/socket/readwrite/*",           find_socket_readwrite);
	dep_add("libs/socket/ntoh/*",                find_socket_ntoh);

	dep_add("libs/socket/getaddrinfo/*",         find_socket_getaddrinfo);
	dep_add("libs/socket/getnameinfo/*",         find_socket_getnameinfo);

	dep_add("libs/socket/inet_addr/*",           find_socket_inetaddr);
	dep_add("libs/socket/gethostbyname/*",       find_socket_gethostbyname);
	dep_add("libs/socket/gethostname/*",         find_socket_gethostname);
	dep_add("libs/socket/in_addr_t/*",           find_socket_inaddrt);
	dep_add("libs/socket/in_port_t/*",           find_socket_inportt);
	dep_add("libs/socket/socklen_t/*",           find_socket_socklent);
	dep_add("libs/socket/sa_family_t/*",         find_socket_safamilyt);
}

int try_socket_(int logdepth, const char *includes_, const char *test_c, const char *cflags, const char *ldflags)
{
	char *out = NULL, *s, *includes;
	char *src;
	int res;
	int ilen, slen, n;

	includes = uniq_inc_str(includes_, NULL, "\n", 0, 0, NULL);
	logprintf(logdepth, "trying '%s' and '%s' and '%s'\n", safeNULL(cflags), safeNULL(ldflags), safeNULL(includes));
	ilen = strlen(includes);
	slen = strlen(test_c);
	src = malloc(ilen + slen + 1);
	memcpy(src, includes, ilen);
	free(includes);
	for(s = src+1, n = 1; n < ilen; n++,s++) {
		if ((s[-1] == '\\') && (s[0] == 'n')) {
			s[-1] = '\n';
			s[0]  = '\n';
		}
	}
	memcpy(src+ilen, test_c, slen);
	src[ilen+slen]='\0';
	res = compile_run(logdepth+1, src, NULL, cflags, ldflags, &out);
	free(src);
	if (res == 0) {
		if (target_emu_fail(out) || (strncmp(out, "OK", 2) == 0)) {
			free(out);
			return 1;
		}
	}
	free(out);
	return 0;
}

/* do not append socket cflags/ldflags - required for detecting socket itself (find_socket_socket()) and for select/poll detection */
int try_socket_pure(int logdepth, const char *prefix, const char *includes_, const char *defs, const char *test_c, const char *cflags, const char *ldflags, int printreport)
{
	char *tmp, *itmp, *incdefs;
	int ilen, ret;
	char *so;
	const char *si;

	incdefs = str_concat(includes_, "\n", defs == NULL ? "" : defs, "\n", NULL);
	ret = try_socket_(logdepth, incdefs, test_c, cflags, ldflags);
	free(incdefs);
	if (ret) {
			char *includes;
			includes = uniq_inc_str(includes_, NULL, "\n", 0, 0, NULL);
			ilen = strlen(includes);

			tmp = malloc(strlen(prefix) + 32);
			sprintf(tmp, "%s/cflags", prefix);
			put(tmp, str_null(cflags));
			sprintf(tmp, "%s/ldflags", prefix);
			put(tmp, str_null(ldflags));
			sprintf(tmp, "%s/presents", prefix);
			put(tmp, "true");

			itmp = malloc(ilen * 2);
			for(si = includes, so = itmp; *si != '\0'; si++, so++) {
				if (*si == '\n') {
					*so = '\\';
					so++;
					*so = 'n';
				}
				else
					*so = *si;
			}
			*so = '\0';
			sprintf(tmp, "%s/includes", prefix);
			put(tmp, itmp);
			if (printreport)
				report("OK ('%s' and '%s' and '%s')\n", str_null(cflags), str_null(ldflags), itmp);
			free(itmp);
			free(tmp);
			free(includes);
			return 1;
		}
	return 0;
}

int try_socket_def(int logdepth, const char *prefix, const char *includes, char *defs_, const char *test_c, const char *cflags, const char *ldflags, int printreport)
{
	const char *sinc, *scflags, *sldflags, *sinit, *suninit;
	char *ninc, *ncflags, *nldflags, *defs;
	int res;

	sinc     = get("libs/socket/socket/includes");
	sinit    = get("libs/socket/socket/init_code");
	suninit  = get("libs/socket/socket/uninit_code");
	scflags  = get("libs/socket/socket/cflags");
	sldflags = get("libs/socket/socket/ldflags");
	if (*includes == '!')
		ninc     = str_concat("\n", (includes+1), "\n",  sinc == NULL ? "" : sinc, NULL);
	else
		ninc     = str_concat("\n", sinc == NULL ? "" : sinc, "\n", includes, NULL);

	defs     = str_concat("", "\n", ((defs_ == NULL) ? "" : defs_), "\n", "#define SOCK_INIT ", sinit, "\n", "#define SOCK_UNINIT ", suninit, "\n", NULL);
	ncflags  = str_concat(" ", cflags, " ", scflags, NULL);
	nldflags = str_concat(" ", ldflags, " ", sldflags, NULL);
	res = try_socket_pure(logdepth, prefix, ninc, defs, test_c, ncflags, nldflags, printreport);
	free(ninc);
	free(ncflags);
	free(nldflags);

	return res;
}

int try_socket(int logdepth, const char *prefix, const char *includes, const char *test_c, const char *cflags, const char *ldflags, int printreport)
{
	return try_socket_def(logdepth, prefix, includes, NULL, test_c, cflags, ldflags, printreport);
}
