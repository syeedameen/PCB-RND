#ifndef RND_HID_MENU_H
#define RND_HID_MENU_H

#include <librnd/core/hid_cfg.h>
#include <genvector/vtp0.h>

typedef struct {
	char *cookie, *desc;
	rnd_hid_cfg_t cfg;
	int prio;
	long uid;
	unsigned has_file:1; /* loaded from a file, can be reloaded any time */
	unsigned loaded_for_conf:1; /* loaded for rc.menu_patches */

	/* internal/cache */
	unsigned cfg_found:1;
} rnd_menu_patch_t;

typedef struct {
	vtp0_t patches; /* list of (rnd_menu_patch_t *), ordered by priority, ascending */
	rnd_hid_cfg_t *merged;
	long changes, last_merged; /* if changes > last_merged, we need to merge */
	int inhibit;
	unsigned gui_ready:1; /* ready for the first merge */
	unsigned gui_nomod:1; /* do the merge but do not send any modification request - useful for the initial menu setup */
	unsigned alloced:1;   /* whether ->merged is alloced (it is not, for the special case of patches->used <= 1 at the time of merging) */
} rnd_menu_sys_t;


/* Search and load the menu file called from fn, using the menu search
   path (from the conf system) if not given by an absolute path; if NULL or
   not found, parse embedded_fallback instead (if it is not NULL).
   Prio is ignored when loading a menu patch file with priority specified in the file.
   Returns NULL on error. */
rnd_menu_patch_t *rnd_hid_menu_load(rnd_hid_t *hid, rnd_hidlib_t *hidlib, const char *cookie, int prio, const char *fn, int exact_fn, const char *embedded_fallback, const char *desc);

/* Unload a menu patch by cookie */
void rnd_hid_menu_unload(rnd_hid_t *hid, const char *cookie);
void rnd_hid_menu_unload_patch(rnd_hid_t *hid, rnd_menu_patch_t *mp);

/* inhibit menu merging: optimization for batch loading/unloading so only
   one merge happens at the end */
void rnd_hid_menu_merge_inhibit_inc(void);
void rnd_hid_menu_merge_inhibit_dec(void);


/* The GUI announces that it is ready for creating the menu; the initial
   merge should happen, but no modification callbacks are done */
void rnd_hid_menu_gui_ready_to_create(rnd_hid_t *hid);

/* The GUI announces that it is ready for executing menu modification callbacks */
void rnd_hid_menu_gui_ready_to_modify(rnd_hid_t *hid);


lht_node_t *rnd_hid_cfg_get_menu(rnd_hid_cfg_t *hr, const char *menu_path);
lht_node_t *rnd_hid_cfg_get_menu_at(rnd_hid_cfg_t *hr, lht_node_t *at, const char *menu_path, lht_node_t *(*cb)(void *ctx, lht_node_t *node, const char *path, int rel_level), void *ctx);
lht_node_t *rnd_hid_cfg_get_menu_at_node(lht_node_t *at, const char *menu_path, lht_node_t *(*cb)(void *ctx, lht_node_t *node, const char *path, int rel_level), void *ctx);

/* plugins can manually create dynamic menus using this call; props->cookie
   must be set, the menu patch is identified by that cookie */
int rnd_hid_menu_create(const char *path, const rnd_menu_prop_t *props);

/* Fields are retrieved using this enum so that HIDs don't need to hardwire
   lihata node names */
typedef enum {
	PCB_MF_ACCELERATOR,
	PCB_MF_SUBMENU,
	PCB_MF_CHECKED,
	PCB_MF_UPDATE_ON,
	PCB_MF_SENSITIVE,
	PCB_MF_TIP,
	PCB_MF_ACTIVE,
	PCB_MF_ACTION,
	PCB_MF_FOREGROUND,
	PCB_MF_BACKGROUND,
	PCB_MF_FONT
} pcb_hid_cfg_menufield_t;

/* Return a field of a submenu and optionally fill in field_name with the
   field name expected in the lihata document (useful for error messages) */
lht_node_t *pcb_hid_cfg_menu_field(const lht_node_t *submenu, pcb_hid_cfg_menufield_t field, const char **field_name);

/* special value for indicating that the new menu node should be inserted on
   top, as the first item (when passed in the ins_after argument */
extern lht_node_t *rnd_hid_menu_ins_as_first;

/* expose the menu system for the preferences dialog */
extern rnd_menu_sys_t rnd_menu_sys;

#endif
