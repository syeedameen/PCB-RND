#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libs.h"
#include "log.h"
#include "db.h"
#include "dep.h"

int try_socket_(int logdepth, const char *includes, const char *test_c, const char *cflags, const char *ldflags);

/* try test program without fetching any implicits */
int try_socket_pure(int logdepth, const char *prefix, const char *includes, const char *defs, const char *test_c, const char *cflags, const char *ldflags, int printreport);

/* try test program with socket init/uninit defines, includes, ldflags and cflags appended */
int try_socket(int logdepth, const char *prefix, const char *includes, const char *test_c, const char *cflags, const char *ldflags, int printreport);

/* try test program with socket init/uninit defines, includes, special defines, ldflags and cflags appended */
int try_socket_def(int logdepth, const char *prefix, const char *includes, char *defs_, const char *test_c, const char *cflags, const char *ldflags, int printreport);

/* socket API detection */
int find_socket_socket(const char *name, int logdepth, int fatal);
int find_socket_wsasocket(const char *name, int logdepth, int fatal);
int find_socket_socketpair(const char *name, int logdepth, int fatal);
int find_socket_select(const char *name, int logdepth, int fatal);
int find_socket_poll(const char *name, int logdepth, int fatal);
int find_socket_types(const char *name, int logdepth, int fatal);
int find_socket_ioctlsocket(const char *name, int logdepth, int fatal);
int find_socket_closesocket(const char *name, int logdepth, int fatal);
int find_socket_SHUT(const char *name, int logdepth, int fatal);
int find_socket_SD(const char *name, int logdepth, int fatal);
int find_socket_shutdown(const char *name, int logdepth, int fatal);
int find_socket_fionbio(const char *name, int logdepth, int fatal);
int find_socket_sockaddr_in(const char *name, int logdepth, int fatal);
int find_socket_lac(const char *name, int logdepth, int fatal);
int find_socket_recvsend(const char *name, int logdepth, int fatal);
int find_socket_readwrite(const char *name, int logdepth, int fatal);
int find_socket_ntoh(const char *name, int logdepth, int fatal);

int find_socket_getaddrinfo(const char *name, int logdepth, int fatal);
int find_socket_getnameinfo(const char *name, int logdepth, int fatal);

int find_socket_inetaddr(const char *name, int logdepth, int fatal);
int find_socket_gethostbyname(const char *name, int logdepth, int fatal);
int find_socket_gethostname(const char *name, int logdepth, int fatal);
int find_socket_inaddrt(const char *name, int logdepth, int fatal);
int find_socket_inportt(const char *name, int logdepth, int fatal);
int find_socket_socklent(const char *name, int logdepth, int fatal);
int find_socket_safamilyt(const char *name, int logdepth, int fatal);
