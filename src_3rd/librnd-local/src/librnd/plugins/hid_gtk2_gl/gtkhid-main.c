#include "config.h"

#include <stdlib.h>

#include <librnd/core/plugins.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hidlib_conf.h>

#include <librnd/plugins/lib_hid_gl/draw_gl.h>

#include <librnd/plugins/lib_gtk_common/glue_common.h>
#include <librnd/plugins/lib_gtk_common/glue_hid.h>

const char *ghid_gl_cookie = "gtk2 hid, gl";

rnd_hid_t gtk2_gl_hid;

extern void ghid_gl_install(pcb_gtk_impl_t *impl, rnd_hid_t *hid);

int gtk2_gl_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	ghid_glue_common_init(ghid_gl_cookie);
	ghid_gl_install(&ghidgui->impl, hid);
	return gtkhid_parse_arguments(hid, argc, argv);
}

int pplg_check_ver_hid_gtk2_gl(int ver_needed) { return 0; }

void pplg_uninit_hid_gtk2_gl(void)
{
	rnd_event_unbind_allcookie(ghid_gl_cookie);
	rnd_conf_hid_unreg(ghid_gl_cookie);
	drawgl_uninit();
}


int pplg_init_hid_gtk2_gl(void)
{
	RND_API_CHK_VER;

	ghid_glue_hid_init(&gtk2_gl_hid);

	gtk2_gl_hid.parse_arguments = gtk2_gl_parse_arguments;

	gtk2_gl_hid.name = "gtk2_gl";
	gtk2_gl_hid.description = "Gtk2 - The Gimp Toolkit, with opengl rendering";

	rnd_hid_register_hid(&gtk2_gl_hid);

	return 0;
}
