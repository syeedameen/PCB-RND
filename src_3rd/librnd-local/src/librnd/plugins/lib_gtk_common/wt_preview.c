/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Alain Vigne
 *  pcb-rnd Copyright (C) 2017..2019 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* This file copied and modified by Peter Clifton, starting from
   gui-pinout-window.c, written by Bill Wilson for the PCB Gtk port,
   then got a major refactoring by Tibor 'Igor2' Palinkas and Alain in pcb-rnd */

#include "config.h"
#include <librnd/core/hidlib_conf.h>

#include "in_mouse.h"
#include "in_keyboard.h"
#include "compat.h"
#include "wt_preview.h"

#include <librnd/core/compat_misc.h>
#include <librnd/core/globalconst.h>

static void get_ptr(pcb_gtk_preview_t *preview, rnd_coord_t *cx, rnd_coord_t *cy, gint *xp, gint *yp);

static void perview_update_offs(pcb_gtk_preview_t *preview)
{
	double xf, yf;

	xf = (double)preview->view.width / preview->view.canvas_width;
	yf = (double)preview->view.height / preview->view.canvas_height;
	preview->view.coord_per_px = (xf > yf ? xf : yf);

	preview->xoffs = (rnd_coord_t)(preview->view.width / 2 - preview->view.canvas_width * preview->view.coord_per_px / 2);
	preview->yoffs = (rnd_coord_t)(preview->view.height / 2 - preview->view.canvas_height * preview->view.coord_per_px / 2);
}

static void pcb_gtk_preview_update_x0y0(pcb_gtk_preview_t *preview)
{
	preview->x_min = preview->view.x0;
	preview->y_min = preview->view.y0;
	preview->x_max = preview->view.x0 + preview->view.width;
	preview->y_max = preview->view.y0 + preview->view.height;
	preview->w_pixels = preview->view.canvas_width;
	preview->h_pixels = preview->view.canvas_height;

	perview_update_offs(preview);
}

void pcb_gtk_preview_zoomto(pcb_gtk_preview_t *preview, const rnd_box_t *data_view)
{
	int orig = preview->view.inhibit_pan_common;
	preview->view.inhibit_pan_common = 1; /* avoid pan logic for the main window */

	preview->view.width = data_view->X2 - data_view->X1;
	preview->view.height = data_view->Y2 - data_view->Y1;

	if (preview->view.width > preview->view.max_width)
		preview->view.max_width = preview->view.width;
	if (preview->view.height > preview->view.max_height)
		preview->view.max_height = preview->view.height;

	pcb_gtk_zoom_view_win(&preview->view, data_view->X1, data_view->Y1, data_view->X2, data_view->Y2, 0);
	pcb_gtk_preview_update_x0y0(preview);
	preview->view.inhibit_pan_common = orig;
}

/* modify the zoom level to coord_per_px (clamped), keeping window cursor
   position wx;wy at perview rnd_coord_t position cx;cy */
void pcb_gtk_preview_zoom_cursor(pcb_gtk_preview_t *preview, rnd_coord_t cx, rnd_coord_t cy, int wx, int wy, double coord_per_px)
{
	int orig;

	coord_per_px = pcb_gtk_clamp_zoom(&preview->view, coord_per_px);

	if (coord_per_px == preview->view.coord_per_px)
		return;

	orig = preview->view.inhibit_pan_common;
	preview->view.inhibit_pan_common = 1; /* avoid pan logic for the main window */
	preview->view.coord_per_px = coord_per_px;

	preview->view.width = preview->view.canvas_width * preview->view.coord_per_px;
	preview->view.height = preview->view.canvas_height * preview->view.coord_per_px;

	if (preview->view.width > preview->view.max_width)
		preview->view.max_width = preview->view.width;
	if (preview->view.height > preview->view.max_height)
		preview->view.max_height = preview->view.height;

	preview->view.x0 = cx - wx * preview->view.coord_per_px;
	preview->view.y0 = cy - wy * preview->view.coord_per_px;

	pcb_gtk_preview_update_x0y0(preview);
	preview->view.inhibit_pan_common = orig;
}

void pcb_gtk_preview_zoom_cursor_rel(pcb_gtk_preview_t *preview, rnd_coord_t cx, rnd_coord_t cy, int wx, int wy, double factor)
{
	pcb_gtk_preview_zoom_cursor(preview, cx, cy, wx, wy, preview->view.coord_per_px * factor);
}

static void preview_set_view(pcb_gtk_preview_t *preview)
{
	rnd_box_t view;

	memcpy(&view, preview->obj, sizeof(rnd_box_t)); /* assumes the object's first field is rnd_box_t */

	pcb_gtk_preview_zoomto(preview, &view);
}

static void preview_set_data(pcb_gtk_preview_t *preview, void *obj)
{
	preview->obj = obj;
	if (obj == NULL) {
		preview->w_pixels = 0;
		preview->h_pixels = 0;
		return;
	}

	preview_set_view(preview);
}

enum {
	PROP_GPORT = 2,
	PROP_INIT_WIDGET = 3,
	PROP_EXPOSE = 4,
	PROP_KIND = 5,
	PROP_COM = 7,
	PROP_DIALOG_DRAW = 8, /* for PCB_LYT_DIALOG */
	PROP_DRAW_DATA = 9,
	PROP_CONFIG = 10
};

static GObjectClass *ghid_preview_parent_class = NULL;

/* Initialises the preview object once it is constructed. Chains up in case
   the parent class wants to do anything too. object is the preview object. */
static void ghid_preview_constructed(GObject *object)
{
	if (G_OBJECT_CLASS(ghid_preview_parent_class)->constructed != NULL)
		G_OBJECT_CLASS(ghid_preview_parent_class)->constructed(object);
}

/* Just before the pcb_gtk_preview_t GObject is finalized, free our
   allocated data, and then chain up to the parent's finalize handler. */
static void ghid_preview_finalize(GObject *object)
{
	pcb_gtk_preview_t *preview = PCB_GTK_PREVIEW(object);

	/* Passing NULL for subcircuit data will clear the preview */
	preview_set_data(preview, NULL);

	G_OBJECT_CLASS(ghid_preview_parent_class)->finalize(object);
}

static void ghid_preview_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	pcb_gtk_preview_t *preview = PCB_GTK_PREVIEW(object);
	GdkWindow *window = gtkc_widget_get_window(GTK_WIDGET(preview));

	switch (property_id) {
	case PROP_GPORT:
		preview->gport = (void *) g_value_get_pointer(value);
		break;
	case PROP_COM:
		preview->ctx = (void *) g_value_get_pointer(value);
		break;
	case PROP_INIT_WIDGET:
		preview->init_drawing_widget = (void *) g_value_get_pointer(value);
		break;
	case PROP_EXPOSE:
		preview->expose = (void *) g_value_get_pointer(value);
		break;
	case PROP_DRAW_DATA:
		preview->expose_data.draw_data = g_value_get_pointer(value);
		if (window != NULL)
			gdk_window_invalidate_rect(window, NULL, FALSE);
		break;
	case PROP_DIALOG_DRAW:
		preview->expose_data.expose_cb = (void *) g_value_get_pointer(value);
		break;
	case PROP_CONFIG:
		preview->config_cb = (void *) g_value_get_pointer(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void ghid_preview_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

#define flip_apply(preview) \
do { \
	if (preview->flip_local) { \
		rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, preview->view.flip_x); \
		rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, preview->view.flip_y); \
	} \
	else if (!preview->flip_global) { \
		rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, 0); \
		rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, 0); \
	} \
} while(0)

/* Converter: set up a pinout expose and use the generic preview expose call */
static gboolean ghid_preview_expose(GtkWidget *widget, pcb_gtk_expose_t *ev)
{
	pcb_gtk_preview_t *preview = PCB_GTK_PREVIEW(widget);
	gboolean res;
	int save_fx, save_fy;

	preview->expose_data.view.X1 = preview->x_min;
	preview->expose_data.view.Y1 = preview->y_min;
	preview->expose_data.view.X2 = preview->x_max;
	preview->expose_data.view.Y2 = preview->y_max;
	save_fx = rnd_conf.editor.view.flip_x;
	save_fy = rnd_conf.editor.view.flip_y;
	flip_apply(preview);

	res = preview->expose(widget, ev, rnd_expose_preview, &preview->expose_data);

	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, save_fx);
	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, save_fy);

	return res;
}

/* Override parent virtual class methods as needed and register GObject properties. */
static void ghid_preview_class_init(pcb_gtk_preview_class_t *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *gtk_widget_class = GTK_WIDGET_CLASS(klass);

	gobject_class->finalize = ghid_preview_finalize;
	gobject_class->set_property = ghid_preview_set_property;
	gobject_class->get_property = ghid_preview_get_property;
	gobject_class->constructed = ghid_preview_constructed;

	PCB_GTK_EXPOSE_EVENT_SET(gtk_widget_class, ghid_preview_expose);

	ghid_preview_parent_class = (GObjectClass *) g_type_class_peek_parent(klass);

	g_object_class_install_property(gobject_class, PROP_GPORT, g_param_spec_pointer("gport", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_COM, g_param_spec_pointer("ctx", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_INIT_WIDGET, g_param_spec_pointer("init-widget", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_EXPOSE, g_param_spec_pointer("expose", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_DIALOG_DRAW, g_param_spec_pointer("dialog_draw", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_DRAW_DATA, g_param_spec_pointer("draw_data", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_CONFIG, g_param_spec_pointer("config", "", "", G_PARAM_WRITABLE));
}

static void update_expose_data(pcb_gtk_preview_t *prv)
{
	pcb_gtk_zoom_post(&prv->view);
	prv->expose_data.view.X1 = prv->view.x0;
	prv->expose_data.view.Y1 = prv->view.y0;
	prv->expose_data.view.X2 = prv->view.x0 + prv->view.width;
	prv->expose_data.view.Y2 = prv->view.y0 + prv->view.height;

/*	rnd_printf("EXPOSE_DATA: %$mm %$mm - %$mm %$mm (%f %$mm)\n",
		prv->expose_data.view.X1, prv->expose_data.view.Y1,
		prv->expose_data.view.X2, prv->expose_data.view.Y2,
		prv->view.coord_per_px, prv->view.x0);*/
}


static gboolean preview_configure_event_cb(GtkWidget *w, GdkEventConfigure *ev, void *tmp)
{
	int need_rezoom;
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *) w;
	preview->win_w = ev->width;
	preview->win_h = ev->height;

	need_rezoom = (preview->view.canvas_width == 0) || (preview->view.canvas_height == 0);

	preview->view.canvas_width = ev->width;
	preview->view.canvas_height = ev->height;

	if (need_rezoom) {
		rnd_box_t b;
		b.X1 = b.Y1 = 0;
		b.X2 = preview->view.width;
		b.Y2 = preview->view.height;
		pcb_gtk_preview_zoomto(preview, &b);
	}
	perview_update_offs(preview);

	if (preview->config_cb != NULL)
		preview->config_cb(preview, w);

/*	update_expose_data(preview);*/
	return TRUE;
}


static gboolean button_press_(GtkWidget *w, rnd_hid_cfg_mod_t btn)
{
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *) w;
	rnd_coord_t cx, cy;
	gint wx, wy;
	get_ptr(preview, &cx, &cy, &wx, &wy);
	void *draw_data = NULL;

	draw_data = preview->expose_data.draw_data;

	switch (btn) {
	case RND_MB_LEFT:
		if (preview->mouse_cb != NULL) {
/*				rnd_printf("bp %mm %mm\n", cx, cy); */
			if (preview->mouse_cb(w, draw_data, RND_HID_MOUSE_PRESS, cx, cy))
				gtk_widget_queue_draw(w);
		}
		break;
	case RND_MB_MIDDLE:
		preview->view.panning = 1;
		preview->grabx = cx;
		preview->graby = cy;
		preview->grabt = time(NULL);
		preview->grabmot = 0;
		break;
	case RND_MB_SCROLL_UP:
		pcb_gtk_preview_zoom_cursor_rel(preview, cx, cy, wx, wy, 0.8);
		goto do_zoom;
	case RND_MB_SCROLL_DOWN:
		pcb_gtk_preview_zoom_cursor_rel(preview, cx, cy, wx, wy, 1.25);
		goto do_zoom;
	default:
		return FALSE;
	}
	return FALSE;

do_zoom:;
	update_expose_data(preview);
	gtk_widget_queue_draw(w);

	return FALSE;
}

static gboolean button_press(GtkWidget *w, rnd_hid_cfg_mod_t btn)
{
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *)w;
	int save_fx, save_fy;
	gboolean r;

	save_fx = rnd_conf.editor.view.flip_x;
	save_fy = rnd_conf.editor.view.flip_y;
	flip_apply(preview);

	r = button_press_(w, btn);

	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, save_fx);
	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, save_fy);

	return r;
}

static gboolean preview_button_press_cb(GtkWidget *w, GdkEventButton *ev, gpointer data)
{
	return button_press(w, ghid_mouse_button(ev->button));
}

static gboolean preview_scroll_cb(GtkWidget *w, GdkEventScroll *ev, gpointer data)
{
	gtk_widget_grab_focus(w);

	switch (ev->direction) {
	case GDK_SCROLL_UP:
		return button_press(w, RND_MB_SCROLL_UP);
	case GDK_SCROLL_DOWN:
		return button_press(w, RND_MB_SCROLL_DOWN);
	default:;
	}

	return FALSE;
}

static gboolean preview_button_release_cb(GtkWidget *w, GdkEventButton *ev, gpointer data)
{
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *) w;
	gint wx, wy;
	rnd_coord_t cx, cy;
	void *draw_data = NULL;
	int save_fx, save_fy;

	save_fx = rnd_conf.editor.view.flip_x;
	save_fy = rnd_conf.editor.view.flip_y;
	flip_apply(preview);

	draw_data = preview->expose_data.draw_data;

	get_ptr(preview, &cx, &cy, &wx, &wy);

	switch (ghid_mouse_button(ev->button)) {
	case RND_MB_MIDDLE:
		preview->view.panning = 0;
		break;
	case RND_MB_RIGHT:
		if ((preview->mouse_cb != NULL) && (preview->mouse_cb(w, draw_data, RND_HID_MOUSE_POPUP, cx, cy)))
			gtk_widget_queue_draw(w);
		break;
	case RND_MB_LEFT:
		if (preview->mouse_cb != NULL) {
/*				rnd_printf("br %mm %mm\n", cx, cy); */
			if (preview->mouse_cb(w, draw_data, RND_HID_MOUSE_RELEASE, cx, cy))
				gtk_widget_queue_draw(w);
		}
		break;
	default:;
	}

	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, save_fx);
	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, save_fy);

	gtk_widget_grab_focus(w);

	return FALSE;
}

static gboolean preview_motion_cb(GtkWidget *w, GdkEventMotion *ev, gpointer data)
{
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *) w;
	int save_fx, save_fy;
	rnd_coord_t cx, cy;
	gint wx, wy;
	void *draw_data = NULL;

	save_fx = rnd_conf.editor.view.flip_x;
	save_fy = rnd_conf.editor.view.flip_y;
	flip_apply(preview);

	draw_data = preview->expose_data.draw_data;

	get_ptr(preview, &cx, &cy, &wx, &wy);
	if (preview->view.panning) {
		preview->grabmot++;
		preview->view.x0 = preview->grabx - wx * preview->view.coord_per_px;
		preview->view.y0 = preview->graby - wy * preview->view.coord_per_px;
		pcb_gtk_preview_update_x0y0(preview);
		update_expose_data(preview);
		gtk_widget_queue_draw(w);
	}
	else if (preview->mouse_cb != NULL) {
		if (preview->mouse_cb(w, draw_data, RND_HID_MOUSE_MOTION, cx, cy))
			gtk_widget_queue_draw(w);
	}

	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, save_fx);
	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, save_fy);

	return FALSE;
}

#include <gdk/gdkkeysyms.h>

static gboolean preview_key_any_cb(GtkWidget *w, GdkEventKey *kev, gpointer data, rnd_bool release)
{
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *)w;
	int mods;
	unsigned short int key_raw, key_tr;

	if ((preview->key_cb == NULL) || (rnd_gtk_key_translate(kev, &mods, &key_raw, &key_tr) != 0))
		return FALSE;

	if (preview->flip_local && release) {
		if (kev->keyval == PCB_GTK_KEY(Tab)) {
			rnd_box_t box;

			box.X1 = preview->view.x0;
			if (preview->view.flip_y)
				box.Y1 = preview->view.ctx->hidlib->size_y - (preview->view.y0 + preview->view.height);
			else
				box.Y1 = preview->view.y0;
			box.X2 = box.X1 + preview->view.width;
			box.Y2 = box.Y1 + preview->view.height;

			preview->view.flip_y = !preview->view.flip_y;

			pcb_gtk_preview_zoomto(preview, &box);
			gtk_widget_queue_draw(w);
		}
	}

	if (preview->key_cb(w, preview->expose_data.draw_data, release, mods, key_raw, key_tr))
		gtk_widget_queue_draw(w);

	return TRUE;
}

static gboolean preview_key_press_cb(GtkWidget *w, GdkEventKey *kev, gpointer data)
{
	return preview_key_any_cb(w, kev, data, 0);
}

static gboolean preview_key_release_cb(GtkWidget *w, GdkEventKey *kev, gpointer data)
{
	return preview_key_any_cb(w, kev, data, 1);
}

/* API */

GType pcb_gtk_preview_get_type()
{
	static GType ghid_preview_type = 0;

	if (!ghid_preview_type) {
		static const GTypeInfo ghid_preview_info = {
			sizeof(pcb_gtk_preview_class_t),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) ghid_preview_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(pcb_gtk_preview_t),
			0,    /* n_preallocs */
			NULL, /* instance_init */
		};

		ghid_preview_type =
			g_type_register_static(GTK_TYPE_DRAWING_AREA, "pcb_gtk_preview_t", &ghid_preview_info, (GTypeFlags) 0);
	}

	return ghid_preview_type;
}

static gint preview_destroy_cb(GtkWidget *widget, gpointer data)
{
	pcb_gtk_t *ctx = data;
	pcb_gtk_preview_t *prv = PCB_GTK_PREVIEW(widget);

	pcb_gtk_preview_del(ctx, prv);
	return 0;
}


GtkWidget *pcb_gtk_preview_new(pcb_gtk_t *ctx, pcb_gtk_init_drawing_widget_t init_widget,
																			pcb_gtk_preview_expose_t expose, rnd_hid_expose_t dialog_draw, pcb_gtk_preview_config_t config, void *draw_data)
{
	pcb_gtk_preview_t *prv = (pcb_gtk_preview_t *)g_object_new(
		PCB_GTK_TYPE_PREVIEW,
		"ctx", ctx, "gport", ctx->impl.gport, "init-widget", init_widget,
		"expose", expose, "dialog_draw", dialog_draw,
		"config", config, "draw_data", draw_data,
		"width-request", 50, "height-request", 50,
		NULL);
	prv->init_drawing_widget(GTK_WIDGET(prv), prv->gport);


TODO(": maybe expose these through the object API so the caller can set it up?")
	memset(&prv->view, 0, sizeof(prv->view));
	prv->view.width = RND_MM_TO_COORD(110);
	prv->view.height = RND_MM_TO_COORD(110);
	prv->view.local_flip = 1;
	prv->view.use_max_pcb = 0;
	prv->view.max_width = RND_MAX_COORD;
	prv->view.max_height = RND_MAX_COORD;
	prv->view.coord_per_px = RND_MM_TO_COORD(0.25);
	prv->view.ctx = ctx;

	update_expose_data(prv);

	prv->init_drawing_widget(GTK_WIDGET(prv), prv->gport);

	gtk_widget_add_events(GTK_WIDGET(prv), GDK_EXPOSURE_MASK | GDK_SCROLL_MASK
		| GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK | GDK_BUTTON_RELEASE_MASK
		| GDK_BUTTON_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_KEY_PRESS_MASK
		| GDK_FOCUS_CHANGE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);


	g_signal_connect(G_OBJECT(prv), "button_press_event", G_CALLBACK(preview_button_press_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "button_release_event", G_CALLBACK(preview_button_release_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "scroll_event", G_CALLBACK(preview_scroll_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "configure_event", G_CALLBACK(preview_configure_event_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "motion_notify_event", G_CALLBACK(preview_motion_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "destroy", G_CALLBACK(preview_destroy_cb), ctx);

	g_signal_connect(G_OBJECT(prv), "key_press_event", G_CALLBACK(preview_key_press_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "key_release_event", G_CALLBACK(preview_key_release_cb), NULL);

	/* keyboard handling needs focusable */
	GTK_WIDGET_SET_FLAGS(prv, GTK_CAN_FOCUS);

	gdl_insert(&ctx->previews, prv, link);
	return GTK_WIDGET(prv);
}

void pcb_gtk_preview_get_natsize(pcb_gtk_preview_t *preview, int *width, int *height)
{
	*width = preview->w_pixels;
	*height = preview->h_pixels;
}

/* Has to be at the end since it undef's SIDE_X */
static void get_ptr(pcb_gtk_preview_t *preview, rnd_coord_t *cx, rnd_coord_t *cy, gint *xp, gint *yp)
{
	gdkc_window_get_pointer(GTK_WIDGET(preview), xp, yp, NULL);
#undef SIDE_X_
#undef SIDE_Y_
#define SIDE_X_(flip, hidlib, x) x
#define SIDE_Y_(flip, hidlib, y) y
	*cx = EVENT_TO_PCB_X(&preview->view, *xp) + preview->xoffs;
	*cy = EVENT_TO_PCB_Y(&preview->view, *yp) + preview->yoffs;
#undef SIDE_X_
#undef SIDE_Y_
}

void pcb_gtk_preview_invalidate(pcb_gtk_t *ctx, const rnd_box_t *screen)
{
	pcb_gtk_preview_t *prv;

	for(prv = gdl_first(&ctx->previews); prv != NULL; prv = prv->link.next) {
		if (!prv->redraw_with_board || prv->redrawing) continue;
		if (screen != NULL) {
			rnd_box_t pb;
			pb.X1 = prv->view.x0;
			pb.Y1 = prv->view.y0;
			pb.X2 = prv->view.x0 + prv->view.width;
			pb.Y2 = prv->view.y0 + prv->view.height;
			if (rnd_box_intersect(screen, &pb)) {
				prv->redrawing = 1;
				ghid_preview_expose(GTK_WIDGET(prv), NULL);
				prv->redrawing = 0;
			}
		}
		else {
			prv->redrawing = 1;
			ghid_preview_expose(GTK_WIDGET(prv), NULL);
			prv->redrawing = 0;
		}
	}
}

void pcb_gtk_previews_flip(pcb_gtk_t *ctx)
{
	pcb_gtk_preview_t *prv;

	for(prv = gdl_first(&ctx->previews); prv != NULL; prv = prv->link.next) {
		if (prv->flip_global) {
			rnd_box_t box;

			box.X1 = prv->view.x0;
			if (!rnd_conf.editor.view.flip_y)
				box.Y1 = prv->view.ctx->hidlib->size_y - (prv->view.y0 + prv->view.height);
			else
				box.Y1 = prv->view.y0;
			box.X2 = box.X1 + prv->view.width;
			box.Y2 = box.Y1 + prv->view.height;

			pcb_gtk_preview_zoomto(prv, &box);
		}
	}
}


void pcb_gtk_preview_del(pcb_gtk_t *ctx, pcb_gtk_preview_t *prv)
{
	if (prv->link.parent == &ctx->previews)
		gdl_remove(&ctx->previews, prv, link);
}
