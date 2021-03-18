int find_posix_fallocate(const char *name, int logdepth, int fatal);
int find_posix_pread(const char *name, int logdepth, int fatal);
int find_posix_pwrite(const char *name, int logdepth, int fatal);
int find_posix_glob(const char *name, int logdepth, int fatal);
int find_posix_fnmatch(const char *name, int logdepth, int fatal);
int find_posix_devzero(const char *name, int logdepth, int fatal);
int find_posix_devnull(const char *name, int logdepth, int fatal);
int find_posix_devrandom(const char *name, int logdepth, int fatal);
int find_posix_devurandom(const char *name, int logdepth, int fatal);
int find_posix_getrusage(const char *name, int logdepth, int fatal);
int find_posix_clockgettime(const char *name, int logdepth, int fatal);
int find_posix_getsid(const char *name, int logdepth, int fatal);
int find_posix_getpid(const char *name, int logdepth, int fatal);
int find_posix_getppid(const char *name, int logdepth, int fatal);
int find_posix_getpgid(const char *name, int logdepth, int fatal);
int find_posix_getuid(const char *name, int logdepth, int fatal);
int find_posix_geteuid(const char *name, int logdepth, int fatal);
int find_posix_getgid(const char *name, int logdepth, int fatal);
int find_posix_getegid(const char *name, int logdepth, int fatal);
int find_posix_posix_spawn(const char *name, int logdepth, int fatal);
int find_posix_openlog(const char *name, int logdepth, int fatal);
int find_posix_syslog(const char *name, int logdepth, int fatal);
int find_posix_vsyslog(const char *name, int logdepth, int fatal);