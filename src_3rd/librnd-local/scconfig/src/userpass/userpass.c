#include "libs.h"
#include "log.h"
#include "db.h"
#include "dep.h"

#include "find_username.h"

void deps_userpass_init()
{
	dep_add("libs/userpass/getpwuid/*",              find_username_getpwuid);
	dep_add("libs/userpass/getpwnam/*",              find_username_getpwnam);
}
