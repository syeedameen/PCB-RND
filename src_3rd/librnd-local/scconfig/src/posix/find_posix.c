#include "find_posix.h"
#include "dep.h"

void deps_posix_init()
{
	dep_add("libs/posix/fs/posix_fallocate/*",         find_posix_fallocate);
	dep_add("libs/posix/fs/pread/*",                   find_posix_pread);
	dep_add("libs/posix/fs/pwrite/*",                  find_posix_pwrite);
	dep_add("libs/posix/fs/glob/*",                    find_posix_glob);
	dep_add("libs/posix/fs/fnmatch/*",                 find_posix_fnmatch);
	dep_add("libs/posix/dev/zero/*",                   find_posix_devzero);
	dep_add("libs/posix/dev/null/*",                   find_posix_devnull);
	dep_add("libs/posix/dev/random/*",                 find_posix_devrandom);
	dep_add("libs/posix/dev/urandom/*",                find_posix_devurandom);
	dep_add("libs/posix/res/getrusage/*",              find_posix_getrusage);
	dep_add("libs/posix/res/clock_gettime/*",          find_posix_clockgettime);
	dep_add("libs/posix/proc/getsid/*",                find_posix_getsid);
	dep_add("libs/posix/proc/getpid/*",                find_posix_getpid);
	dep_add("libs/posix/proc/getppid/*",               find_posix_getppid);
	dep_add("libs/posix/proc/getpgid/*",               find_posix_getpgid);
	dep_add("libs/posix/proc/getuid/*",                find_posix_getuid);
	dep_add("libs/posix/proc/geteuid/*",               find_posix_geteuid);
	dep_add("libs/posix/proc/getgid/*",                find_posix_getgid);
	dep_add("libs/posix/proc/getegid/*",               find_posix_getegid);
	dep_add("libs/posix/proc/posix_spawn/*",           find_posix_posix_spawn);
	dep_add("libs/posix/log/openlog/*",                find_posix_openlog);
	dep_add("libs/posix/log/syslog/*",                 find_posix_syslog);
	dep_add("libs/posix/log/vsyslog/*",                find_posix_vsyslog);
}
