#include <stdio.h>
#include <string.h>
#include "arg.h"
#include "log.h"
#include "dep.h"
#include "libs.h"
#include "db.h"
#include "tmpasm.h"
#include "tmpasm_scconfig.h"
#include "generator.h"
#include "util/arg_auto_set.h"
#include "Rev.h"

#define version "2.3.1"

#define LIBRND_ROOT         "../src/librnd"
#define LIBRND_PLUGIN_ROOT  "../src/librnd/plugins"
#include "../src_3rd/puplug/scconfig_hooks.h"
#include "../src_3rd/libfungw/scconfig_hooks.h"
#include "../src_3rd/libporty_net/hooks_net.c"

#include "librnd/scconfig/plugin_3state.h"
#include "librnd/scconfig/hooks_common.h"
#include "librnd/scconfig/hooks_gui.c"
#include "librnd/scconfig/rnd_hook_detect.h"

const arg_auto_set_t disable_libs[] = { /* list of --disable-LIBs and the subtree they affect */
	{"disable-gd",        "libs/gui/gd",                  arg_lib_nodes, "$do not use gd (many exporters need it)"},
	{"disable-gd-gif",    "libs/gui/gd/gdImageGif",       arg_lib_nodes, "$no gif support in the png rnd_exporter"},
	{"disable-gd-png",    "libs/gui/gd/gdImagePng",       arg_lib_nodes, "$no png support in the png rnd_exporter"},
	{"disable-gd-jpg",    "libs/gui/gd/gdImageJpeg",      arg_lib_nodes, "$no jpeg support in the png rnd_exporter"},
	{"enable-bison",      "/local/pcb/want_bison",        arg_true,      "$enable generating language files using bison/flex"},
	{"disable-bison",     "/local/pcb/want_bison",        arg_false,     "$disable generating language files using bison/flex"},
	{"enable-byaccic",    "/local/pcb/want_byaccic",      arg_true,      "$enable generating language files using byaccic/ureglex"},
	{"disable-byaccic",   "/local/pcb/want_byaccic",      arg_false,     "$disable generating language files byaccic/ureglex"},

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_, hidlib_) plugin3_args(name, desc)
#define plugin_header(sect)
#define plugin_dep(plg, on, hidlib)
#include "plugins.h"

	{NULL, NULL, NULL, NULL}
};

static void help1(void)
{
	rnd_help1("librnd");
}

/* Runs when a custom command line argument is found
 returns true if no further argument processing should be done */
int hook_custom_arg(const char *key, const char *value)
{
	rnd_hook_custom_arg(key, value, disable_libs); /* call arg_auto_print_options() instead */

	if (arg_auto_set(key, value, disable_libs) == 0) {
		fprintf(stderr, "Error: unknown argument %s\n", key);
		exit(1);
	}
	return 1; /* handled by arg_auto_set() */
}

/* Runs before anything else */
int hook_preinit()
{
	return 0;
}

/* Runs after initialization */
int hook_postinit()
{
	db_mkdir("/local");
	db_mkdir("/local/pcb");

	rnd_hook_postinit();

	put("/local/pcb/want_bison", sfalse);
	put("/local/pcb/want_byaccic", sfalse);

	return 0;
}

/* Runs after all arguments are read and parsed */
int hook_postarg()
{
	return rnd_hook_postarg();
}



/* Runs when things should be detected for the host system */
int hook_detect_host()
{
	return rnd_hook_detect_host();
}

/* Runs when things should be detected for the target system */
int hook_detect_target()
{
	int want_gd, want_stroke;

	want_gd     = plug_is_enabled("export_png") || plug_is_enabled("import_pxm_gd");
	want_stroke = plug_is_enabled("stroke");

	plugin_db_hidlib();

	rnd_hook_detect_cc();
	if (rnd_hook_detect_sys() != 0)
		return 1;

	if (want_stroke) {
		require("libs/gui/libstroke/presents", 0, 0);
		if (!istrue(get("libs/gui/libstroke/presents"))) {
			report_repeat("WARNING: Since there's no libstroke found, disabling the stroke plugin...\n");
			hook_custom_arg("disable-stroke", NULL);
		}
	}

	rnd_hook_detect_hid(0);
	rnd_calc_dialog_deps();

	if (want_gd) {
		require("libs/gui/gd/presents", 0, 0);
		if (!istrue(get("libs/gui/gd/presents"))) {
			report_repeat("WARNING: Since there's no libgd, disabling gd based exports and pixmap imports (png, gif, jpeg)...\n");
			hook_custom_arg("disable-gd-gif", NULL);
			hook_custom_arg("disable-gd-png", NULL);
			hook_custom_arg("disable-gd-jpg", NULL);
#ifdef PNG
			hook_custom_arg("disable-export_png", NULL);
			hook_custom_arg("disable-import_pxm_gd", NULL);
#endif
			want_gd = 0;
			goto disable_gd_formats;
		}
		else {
			require("libs/gui/gd/gdImagePng/presents", 0, 0);
			require("libs/gui/gd/gdImageGif/presents", 0, 0);
			require("libs/gui/gd/gdImageJpeg/presents", 0, 0);
			require("libs/gui/gd/gdImageSetResolution/presents", 0, 0);
			if (!istrue(get("libs/gui/gd/gdImagePng/presents"))) {
				report_repeat("WARNING: libgd is installed, but its png code fails, some exporters will be compiled with reduced functionality\n");
			}
		}
	}
	else {
		put("libs/gui/gd/presents", sfalse);
		disable_gd_formats:;
		put("libs/gui/gd/gdImagePng/presents", sfalse);
		put("libs/gui/gd/gdImageGif/presents", sfalse);
		put("libs/gui/gd/gdImageJpeg/presents", sfalse);
	}

	/* yacc/lex - are we able to regenerate languages? */
	if (istrue(get("/local/pcb/want_bison"))) {
		require("parsgen/flex/*", 0, 0);
		require("parsgen/bison/*", 0, 0);
		if (!istrue(get("parsgen/flex/presents")) || !istrue(get("parsgen/bison/presents")))
			put("/local/pcb/want_parsgen", sfalse);
		else
			put("/local/pcb/want_parsgen", strue);
	}
	else {
		report("Bison/flex are disabled, among with parser generation.\n");
		put("/local/pcb/want_parsgen", sfalse);
	}

	/* byaccic - are we able to regenerate languages? */
	if (istrue(get("/local/pcb/want_byaccic"))) {
		require("parsgen/byaccic/*", 0, 0);
		require("parsgen/ureglex/*", 0, 0);
		if (!istrue(get("parsgen/byaccic/presents")) || !istrue(get("parsgen/ureglex/presents")))
			put("/local/pcb/want_parsgen_byaccic", sfalse);
		else
			put("/local/pcb/want_parsgen_byaccic", strue);
	}
	else {
		report("byaccic/ureglex are disabled, among with parser generation.\n");
		put("/local/pcb/want_parsgen_byaccic", sfalse);
	}

	if (libporty_net_detect_target() != 0) {
		hook_custom_arg("disable-lib_portynet", NULL);
		report_repeat("WARNING: porty-net did not configure, some network-related features are disabled\n");
	}

	/* plugin dependencies */
	plugin_deps(1);

	rnd_hook_detect_coord_bits(); /* remove this after the librnd split */

	/* restore the original CFLAGS, without the effects of --debug, so Makefiles can decide when to use what cflag (c99 needs different ones) */
	/* this should be removed after the librnd split */
	put("/target/cc/cflags", get("/local/cc_flags_save"));

	return 0;
}

#ifdef GENCALL
/* If enabled, generator implements ###call *### and generator_callback is
   the callback function that will be executed upon ###call### */
void generator_callback(char *cmd, char *args)
{
	printf("* generator_callback: '%s' '%s'\n", cmd, args);
}
#endif

/* Runs after detection hooks, should generate the output (Makefiles, etc.) */
int hook_generate()
{
	char *next, *rev = "non-svn", *curr, *tmp, *sep;
	int generr = 0;
	int res = 0;
	char apiver[32], version_major[32];
	long r = 0;
	int v1, v2, v3;

	tmp = svn_info(0, "../src", "Revision:");
	if (tmp != NULL) {
		r = strtol(tmp, NULL, 10);
		rev = str_concat("", "svn r", tmp, NULL);
		free(tmp);
	}
	strcpy(apiver, version);
	curr = apiver; next = strchr(curr, '.'); *next = '\n';
	v1 = atoi(curr);
	curr = next+1; next = strchr(curr, '.'); *next = '\n';
	v2 = atoi(curr);
	v3 = atoi(next+1);
	sprintf(apiver, "%01d%01d%02d%05ld", v1, v2, v3, r);

	strcpy(version_major, version);
	sep = strchr(version_major, '.');
	if (sep != NULL)
		*sep = '\0';

	logprintf(0, "scconfig generate version info: version='%s' rev='%s'\n", version, rev);
	put("/local/revision", rev);
	put("/local/version",  version);
	put("/local/version_major",  version_major);
	put("/local/apiver", apiver);
	put("/local/pup/sccbox", "../../scconfig/sccbox");

	printf("\n");

	printf("Generating Makefile.conf (%d)\n", generr |= tmpasm("..", "Makefile.conf.in", "Makefile.conf"));

	printf("Generating src/Makefile (%d)\n", generr |= tmpasm("../src", "Makefile.in", "Makefile"));

	generr |= rnd_hook_generate();

	printf("Generating src_3rd/libporty_net/os_includes.h (%d)\n", generr |= generate("../src_3rd/libporty_net/os_includes.h.in", "../src_3rd/libporty_net/os_includes.h"));
	printf("Generating src_3rd/libporty_net/pnet_config.h (%d)\n", generr |= generate("../src_3rd/libporty_net/pnet_config.h.in", "../src_3rd/libporty_net/pnet_config.h"));
	printf("Generating src_3rd/libporty_net/phost_types.h (%d)\n", generr |= generate("../src_3rd/libporty_net/phost_types.h.in", "../src_3rd/libporty_net/phost_types.h"));

	printf("Generating pcb-rnd config.h (%d)\n", generr |= tmpasm("..", "config.h.in", "config.h"));

	printf("Generating opengl.h (%d)\n", generr |= tmpasm("../src/librnd/plugins/lib_hid_gl", "opengl.h.in", "opengl.h"));

	printf("Generating tests/librnd/inc_all.h (%d)\n", generr |= tmpasm("../tests/librnd", "inc_all.h.in", "inc_all.h"));

	generr |= pup_hook_generate("../src_3rd/puplug");

	if (!istrue(get("libs/script/fungw/presents")))
		generr |= fungw_hook_generate("../src_3rd/libfungw");


	if (!generr) {
	printf("\n\n");
	printf("=====================\n");
	printf("Configuration summary\n");
	printf("=====================\n");

	print_sum_setting("/local/pcb/want_parsgen",   "Regenerating languages with bison & flex");
	print_sum_setting("/local/pcb/debug",          "Compilation for debugging");
	print_sum_setting_or("/local/pcb/symbols",     "Include debug symbols", istrue(get("/local/pcb/debug")));
	print_sum_cfg_val("/local/pcb/coord_bits",     "Coordinate type bits");

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_, hidlib_) plugin3_stat(name, desc)
#define plugin_header(sect) printf(sect);
#define plugin_dep(plg, on, hidlib)
#include "plugins.h"

	if (repeat != NULL) {
		printf("\n%s\n", repeat);
	}

	if (all_plugin_check_explicit()) {
		printf("\nNevertheless the configuration is complete, if you accept these differences\nyou can go on compiling.\n\n");
		res = 1;
	}
	else
		printf("\nConfiguration complete, ready to compile.\n\n");

	{
		FILE *f;
		f = fopen("Rev.stamp", "w");
		fprintf(f, "%d", myrev);
		fclose(f);
	}

	}
	else {
		fprintf(stderr, "Error generating some of the files\n");
		res = 1;
	}

	return res;
}

/* Runs before everything is uninitialized */
void hook_preuninit()
{
}

/* Runs at the very end, when everything is already uninitialized */
void hook_postuninit()
{
}

