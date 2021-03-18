#include <stdlib.h>
#include <string.h>
#include "scmenu.h"

scm_menu_entry_t me_en_dis[] = {
	{SCM_KEY_VALUE,       NULL, "enable",  "let it be", NULL, NULL,  SCM_RADIO},
	{SCM_KEY_VALUE,       NULL, "disable", "no way!", NULL, NULL, SCM_RADIO},
	{SCM_TERMINATOR,     NULL, NULL, NULL, NULL, NULL, 0}
};

scm_menu_entry_t me_list[] = {
	{SCM_KEY_ONLY,       NULL, "textbox", NULL, NULL, NULL, 0},
	{SCM_KEY_VALUE,      NULL, "readline", "original text", NULL, NULL, 0},
	{SCM_KEY_ONLY,       NULL, "radio/check", NULL, NULL, NULL, 0},
	{SCM_COMBO,          NULL, "auto combo", "<select one>", NULL, me_en_dis, SCM_AUTO_RUN},
	{SCM_KEY_VALUE_EDIT, NULL, "auto val",   "val4", NULL, NULL, SCM_AUTO_RUN},
	{SCM_SEPARATOR,      NULL, NULL,   NULL, NULL, NULL, 0},
	{SCM_KEY_VALUE_EDIT, NULL, "key5", "", NULL, NULL, 0},
	{SCM_EMPTY,          NULL, NULL,   NULL, NULL, NULL, 0},
	{SCM_KEY_VALUE_EDIT, NULL, "key6", "sdhfaskjdfasdf", NULL, NULL, 0},
	{SCM_TERMINATOR,     NULL, NULL, NULL, NULL, NULL, 0}
};

scm_menu_entry_t me_list2[] = {
	{SCM_KEY_ONLY,       NULL, "c1", NULL, NULL, NULL, SCM_CHECKBOX},
	{SCM_KEY_ONLY,       NULL, "c2", NULL, NULL, NULL, SCM_CHECKBOX | SCM_CHECKED},
	{SCM_KEY_ONLY,       NULL, "c3 long text for checking sizes", NULL, NULL, NULL, SCM_CHECKBOX},
	{SCM_SEPARATOR,      NULL, NULL,   NULL, NULL, NULL, 0},
	{SCM_KEY_ONLY,       NULL, "r1", NULL, NULL, NULL, SCM_RADIO},
	{SCM_KEY_ONLY,       NULL, "r2", NULL, NULL, NULL, SCM_RADIO | SCM_CHECKED},
	{SCM_KEY_ONLY,       NULL, "r3", NULL, NULL, NULL, SCM_RADIO | SCM_CHECKED},
	{SCM_KEY_ONLY,       NULL, "nothing", NULL, NULL, NULL, 0},
	{SCM_TERMINATOR,     NULL, NULL, NULL, NULL, NULL, 0}
};


int menu_cb(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *w, int selection)
{
	int res;
	char *ress;

	switch(selection) {
		case 0:
			res = scm_popup(ctx, "title", "line 1\nline    2\nwhat\nis\nthis", SCM_YES | SCM_CANCEL);
			vt100_move(&ctx->hrl.term, 24, 1);
			printf("res=%d\n", res);
			return -2;
		case 1:
			ress = scm_readline(ctx, "read this line now", "prompt: ", me_list[1].value);
			if (ress != NULL) {
				me_list[1].value = ress;
			}
			return -2;
		case 2:
			{
				scm_menu_t m2;
				scm_win_t w2;

				memset(&w2, 0, sizeof(w2));
				w2.title = "checkbox & radio";

				memset(&m2, 0, sizeof(m2));
				m2.entries = me_list2;
				m2.num_entries = -1;
				m2.cursor = 0;

				scm_menu(ctx, &m2, &w2, NULL);
			}
			return -2;
	}
	return selection;
}

int main()
{
	scm_ctx_t ctx;
	scm_win_t w;
	scm_menu_t m;

	memset(&w, 0, sizeof(w));
	w.title = "main menu";
	w.cfg_h = 5;

	memset(&m, 0, sizeof(m));
	m.entries = me_list;
	m.num_entries = -1;
	m.cursor = 0;

	scm_init(&ctx);
	scm_menu(&ctx, &m, &w, menu_cb);

	return 0;
}
