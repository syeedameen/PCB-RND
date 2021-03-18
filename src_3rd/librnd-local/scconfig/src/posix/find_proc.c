/*
    scconfig - detection of POSIX library features: process specific calls
    Copyright (C) 2018  Tibor Palinkas
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "libs.h"
#include "log.h"
#include "db.h"
#include "dep.h"

static int find_posix_func__(const char *name, int logdepth, int fatal, const char *func, const char *arg, const char *rtype, const char *badval)
{
	char key[100];
	const char *test_c_template =
		NL "#include <stdio.h>"
		NL "int main()"
		NL "{"
		NL "	if (%s(%s) != ((%s)(%s)))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;
	char test_c[1000];

	sprintf(key, "libs/posix/proc/%s", func);
	sprintf(test_c, test_c_template, func, arg, rtype, badval);

	require("cc/cc", logdepth, fatal);

	report("Checking for %s()... ", func);
	logprintf(logdepth, "find_posix_%s: trying to find %s()...\n", func, func);
	logdepth++;

	if (try_icl(logdepth, key, test_c, "#include <unistd.h>", NULL, NULL))
		return 0;
	if (try_icl(logdepth, key, test_c, "#include <sys/types.h>\n#include <unistd.h>", NULL, NULL))
		return 0;

	return try_fail(logdepth, key);
}

int find_posix_getsid(const char *name, int logdepth, int fatal)
{
	return find_posix_func__(name, logdepth, fatal, "getsid", "(pid_t)0", "pid_t", "-1");
}

int find_posix_getpid(const char *name, int logdepth, int fatal)
{
	const char *key = "libs/posix/proc/getpid";
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main()"
		NL "{"
		NL no_implicit(pid_t, "getpid", "getpid")
		NL "	if (getpid() != ((pid_t)(-1)))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for getpid()... ");
	logprintf(logdepth, "find_posix_getpid: trying to find getpid()...\n");
	logdepth++;

	if (try_icl(logdepth, key, test_c, "#include <unistd.h>", NULL, NULL))
		return 0;
	if (try_icl(logdepth, key, test_c, "#include <sys/types.h>\n#include <unistd.h>", NULL, NULL))
		return 0;

	return try_fail(logdepth, key);
}

int find_posix_getppid(const char *name, int logdepth, int fatal)
{
	const char *key = "libs/posix/proc/getppid";
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main()"
		NL "{"
		NL "	if (getppid() != ((pid_t)(-1)))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for getppid()... ");
	logprintf(logdepth, "find_posix_getppid: trying to find getppid()...\n");
	logdepth++;

	if (try_icl(logdepth, key, test_c, "#include <unistd.h>", NULL, NULL))
		return 0;
	if (try_icl(logdepth, key, test_c, "#include <sys/types.h>\n#include <unistd.h>", NULL, NULL))
		return 0;

	return try_fail(logdepth, key);
}

int find_posix_getpgid(const char *name, int logdepth, int fatal)
{
	const char *key = "libs/posix/proc/getpgid";
	const char *test_c =
		NL "#include <stdio.h>"
		NL "int main()"
		NL "{"
		NL "	if (getpgid(getpid()) != ((pid_t)(-1)))"
		NL "		puts(\"OK\");"
		NL "	return 0;"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);
	if (require("libs/posix/proc/getpid", logdepth, fatal) != 0)
		return try_fail(logdepth, key);

	report("Checking for getpgid()... ");
	logprintf(logdepth, "find_posix_getpgid: trying to find getpgid()...\n");
	logdepth++;

	if (try_icl(logdepth, key, test_c, "#include <unistd.h>", NULL, NULL))
		return 0;
	if (try_icl(logdepth, key, test_c, "#include <sys/types.h>\n#include <unistd.h>", NULL, NULL))
		return 0;

	return try_fail(logdepth, key);
}

int find_posix_getuid(const char *name, int logdepth, int fatal)
{
	return find_posix_func__(name, logdepth, fatal, "getuid", "", "uid_t", "-1");
}

int find_posix_geteuid(const char *name, int logdepth, int fatal)
{
	return find_posix_func__(name, logdepth, fatal, "geteuid", "", "uid_t", "-1");
}

int find_posix_getgid(const char *name, int logdepth, int fatal)
{
	return find_posix_func__(name, logdepth, fatal, "getgid", "", "gid_t", "-1");
}

int find_posix_getegid(const char *name, int logdepth, int fatal)
{
	return find_posix_func__(name, logdepth, fatal, "getegid", "", "gid_t", "-1");
}

int find_posix_posix_spawn(const char *name, int logdepth, int fatal)
{
	const char *key = "libs/posix/proc/posix_spawn";
	const char *test_c =
		NL "pid_t my_wait(int *status);"
		NL "extern char **environ;"
		NL "int main()"
		NL "{"
		NL "	pid_t pid;"
		NL "	int st;"
		NL "	char *args[] = { \"sh\", \"-c\", \"echo OK\", (char *)0 };"
		NL "	if (posix_spawnp(&pid, args[0], (void *)0, (void *)0, args, environ) != 0)"
		NL "		return 1;"
		NL "	if (my_wait(&st) == -1)"
		NL "		return 1;"
		NL "	return 0;"
		NL "}"
		NL "#include <sys/types.h>"
		NL "#include <sys/wait.h>"
		NL "pid_t my_wait(int *status)"
		NL "{"
		NL "	return wait(status);"
		NL "}"
		NL;

	require("cc/cc", logdepth, fatal);

	report("Checking for posix_spawn()... ");
	logprintf(logdepth, "find_posix_posix_spawn: trying to find posix_spawn()...\n");
	logdepth++;

	if (try_icl(logdepth, key, test_c, "#include <spawn.h>", NULL, NULL))
		return 0;

	return try_fail(logdepth, key);
}
