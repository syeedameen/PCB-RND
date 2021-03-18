#include "find_sul.h"
#include "dep.h"

void deps_sul_init()
{
	dep_add("libs/sul/glib/*",     find_sul_glib);
	dep_add("libs/sul/gettext/*",  find_sul_gettext);
	dep_add("libs/sul/dmalloc/*",  find_sul_dmalloc);
	dep_add("libs/sul/dbus/*",     find_sul_dbus);
	dep_add("libs/sul/libxml2/*",  find_sul_libxml2);
	dep_add("libs/sul/freetype2/*",find_sul_freetype2);
	dep_add("libs/sul/pcre/*",     find_sul_pcre);
	dep_add("libs/sul/aspell/*",   find_sul_aspell);
	dep_add("libs/sul/regex/*",    find_sul_regex);
	dep_add("libs/sul/libext2fs/*",find_sul_libext2fs);
	dep_add("libs/sul/fuse/*",     find_sul_fuse);
	dep_add("libs/sul/iconv/*",    find_sul_iconv);

	dep_add("libs/sul/genht/*",    find_sul_genht);
	dep_add("libs/sul/genhvbox/*", find_sul_genhvbox);
	dep_add("libs/sul/genvector/*", find_sul_genvector);
	dep_add("libs/sul/gentrex/*", find_sul_gentrex);
	dep_add("libs/sul/genusin/*", find_sul_genusin);
	dep_add("libs/sul/genregex/*", find_sul_genregex);
	dep_add("libs/sul/genlist/*", find_sul_genlist);

	dep_add("libs/sul/librnd-3rd/*",  find_sul_librnd_3rd);
	dep_add("libs/sul/librnd-hid/*",  find_sul_librnd_hid);

}
