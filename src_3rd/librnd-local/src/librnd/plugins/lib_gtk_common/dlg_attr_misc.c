/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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
 */

/* Smallish misc DAD widgets */

#include "wt_preview.h"

#include <librnd/plugins/lib_hid_common/dad_markup.h>

static int ghid_progress_set(attr_dlg_t *ctx, int idx, const rnd_hid_attr_val_t *val)
{
	GtkWidget *prg = ctx->wl[idx];
	double pos = val->dbl;

	if (pos < 0.0) pos = 0.0;
	else if (pos > 1.0) pos = 1.0;

	if ((pos >= 0.0) && (pos <= 1.0)) /* extra case for NaN */
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(prg), pos);
	return 0;
}


static GtkWidget *ghid_progress_create(attr_dlg_t *ctx, rnd_hid_attribute_t *attr, GtkWidget *parent, int j)
{
	GtkWidget *bparent, *prg;

	prg = gtk_progress_bar_new();
	gtk_widget_set_size_request(prg, -1, 16);

	gtk_widget_set_tooltip_text(prg, attr->help_text);
	bparent = frame_scroll(parent, attr->rnd_hatt_flags, &ctx->wltop[j]);
	gtk_box_pack_start(GTK_BOX(bparent), prg, TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(prg), PCB_OBJ_PROP, ctx);
	return prg;
}

static void ghid_preview_expose(rnd_hid_gc_t gc, const rnd_hid_expose_ctx_t *e)
{
	rnd_hid_preview_t *prv = e->draw_data;
	prv->user_expose_cb(prv->attrib, prv, gc, e);
}

static rnd_bool ghid_preview_mouse(void *widget, void *draw_data, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y)
{
	rnd_hid_preview_t *prv = draw_data;
	if (prv->user_mouse_cb != NULL)
		return prv->user_mouse_cb(prv->attrib, prv, kind, x, y);
	return rnd_false;
}

static rnd_bool ghid_preview_key(void *widget, void *draw_data, rnd_bool release, rnd_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr)
{
	rnd_hid_preview_t *prv = draw_data;
	if (prv->user_key_cb != NULL)
		return prv->user_key_cb(prv->attrib, prv, release, mods, key_raw, key_tr);
	return rnd_false;
}

void ghid_preview_zoomto(rnd_hid_attribute_t *attrib, void *hid_ctx, const rnd_box_t *view)
{
	attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	GtkWidget *prv = ctx->wl[idx];
	pcb_gtk_preview_zoomto(PCB_GTK_PREVIEW(prv), view);
	gtk_widget_queue_draw(prv);
}

static int ghid_preview_set(attr_dlg_t *ctx, int idx, const rnd_hid_attr_val_t *val)
{
	GtkWidget *prv = ctx->wl[idx];

	gtk_widget_queue_draw(prv);
	return 0;
}


void ghid_preview_config(pcb_gtk_preview_t *gp, GtkWidget *widget)
{
	rnd_hid_preview_t *prv = gp->expose_data.draw_data;
	if (prv->initial_view_valid) {
		pcb_gtk_preview_zoomto(PCB_GTK_PREVIEW(widget), &prv->initial_view);
		gtk_widget_queue_draw(widget);
		prv->initial_view_valid = 0;
	}
}

static GtkWidget *ghid_preview_create(attr_dlg_t *ctx, rnd_hid_attribute_t *attr, GtkWidget *parent, int j)
{
	GtkWidget *bparent, *prv;
	pcb_gtk_preview_t *p;
	rnd_hid_preview_t *hp = attr->wdata;

	hp->hid_wdata = ctx;
	hp->hid_zoomto_cb = ghid_preview_zoomto;
	
	bparent = frame_scroll(parent, attr->rnd_hatt_flags, &ctx->wltop[j]);
	prv = pcb_gtk_preview_new(ctx->gctx, ctx->gctx->impl.init_drawing_widget, ctx->gctx->impl.preview_expose, ghid_preview_expose, ghid_preview_config, attr->wdata);
	gtk_box_pack_start(GTK_BOX(bparent), prv, TRUE, TRUE, 0);
	gtk_widget_set_tooltip_text(prv, attr->help_text);
	p = (pcb_gtk_preview_t *) prv;
	p->mouse_cb = ghid_preview_mouse;
	p->key_cb = ghid_preview_key;

/*	p->overlay_draw_cb = pcb_stub_draw_csect_overlay;*/
TODO("TODO make these configurable:")
	p->x_min = 0;
	p->y_min = 0;
	p->x_max = RND_MM_TO_COORD(100);
	p->y_max = RND_MM_TO_COORD(100);
	p->w_pixels = RND_MM_TO_COORD(10);
	p->h_pixels = RND_MM_TO_COORD(10);
	p->redraw_with_board = !!(attr->hatt_flags & RND_HATF_PRV_BOARD);

	gtk_widget_set_size_request(prv, hp->min_sizex_px, hp->min_sizey_px);
	return prv;
}

static GtkWidget *ghid_picture_create(attr_dlg_t *ctx, rnd_hid_attribute_t *attr, GtkWidget *parent, int j, GCallback click_cb, void *click_ctx)
{
	GtkWidget *bparent, *pic, *evb;
	GdkPixbuf *pixbuf;
	bparent = frame_scroll(parent, attr->rnd_hatt_flags, &ctx->wltop[j]);
	int expfill = (attr->rnd_hatt_flags & RND_HATF_EXPFILL);

	pixbuf = gdk_pixbuf_new_from_xpm_data(attr->wdata);
	pic = gtk_image_new_from_pixbuf(pixbuf);
	evb = wrap_bind_click(pic, click_cb, attr);
	g_object_set_data(G_OBJECT(evb), PCB_OBJ_PROP, click_ctx);

	gtk_box_pack_start(GTK_BOX(bparent), evb, expfill, expfill, 0);
	gtk_widget_set_tooltip_text(pic, attr->help_text);

	return evb;
}


static GtkWidget *ghid_picbutton_create(attr_dlg_t *ctx, rnd_hid_attribute_t *attr, GtkWidget *parent, int j, int toggle, int expfill)
{
	GtkWidget *bparent, *button, *img;
	GdkPixbuf *pixbuf;

	bparent = frame_scroll(parent, attr->rnd_hatt_flags, &ctx->wltop[j]);

	pixbuf = gdk_pixbuf_new_from_xpm_data(attr->wdata);
	img = gtk_image_new_from_pixbuf(pixbuf);

	if (toggle)
		button = gtk_toggle_button_new();
	else
		button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), img);

	gtk_box_pack_start(GTK_BOX(bparent), button, expfill, expfill, 0);
	gtk_widget_set_tooltip_text(button, attr->help_text);

	return button;
}

static GtkWidget *ghid_color_create(attr_dlg_t *ctx, rnd_hid_attribute_t *attr, GtkWidget *parent, int j)
{
	GtkWidget *bparent, *button;
	pcb_gtk_color_t gclr;

	bparent = frame_scroll(parent, attr->rnd_hatt_flags, &ctx->wltop[j]);

	memset(&gclr, 0, sizeof(gclr));
	ctx->gctx->impl.map_color(rnd_color_black, &gclr);

	button = gtkc_color_button_new_with_color(&gclr);
	gtk_color_button_set_title(GTK_COLOR_BUTTON(button), NULL);

	gtk_box_pack_start(GTK_BOX(bparent), button, TRUE, TRUE, 0);
	gtk_widget_set_tooltip_text(button, attr->help_text);

	return button;
}


static int ghid_color_set(attr_dlg_t *ctx, int idx, const rnd_hid_attr_val_t *val)
{
	pcb_gtk_color_t gclr;
	GtkWidget *btn = ctx->wl[idx];

	memset(&gclr, 0, sizeof(gclr));
	ctx->gctx->impl.map_color(&val->clr, &gclr);
	gtkc_color_button_set_color(btn, &gclr);

	return 0;
}

