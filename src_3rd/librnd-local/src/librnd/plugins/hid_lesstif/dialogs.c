#include "xincludes.h"

#include "config.h"
#include <librnd/core/hidlib_conf.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "FillBox.h"

#include <librnd/core/compat_misc.h>
#include <librnd/core/event.h>
#include <librnd/core/rnd_printf.h>

#include <librnd/core/hid.h>
#include "lesstif.h"
#include <librnd/core/hid_attrib.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_init.h>
#include "ltf_stdarg.h"
#include <librnd/core/misc_util.h>

int pcb_ltf_ok;
extern rnd_hidlib_t *ltf_hidlib;


#define COMPONENT_SIDE_NAME "(top)"
#define SOLDER_SIDE_NAME "(bottom)"

static int wplc_hack = 0;
static int wplc_hackx = 0, wplc_hacky = 0;

void pcb_ltf_winplace(Display *dsp, Window w, const char *id, int defx, int defy)
{
	int plc[4] = {-1, -1, -1, -1};

	plc[2] = defx;
	plc[3] = defy;

	rnd_event(ltf_hidlib, RND_EVENT_DAD_NEW_DIALOG, "psp", NULL, id, plc);

	if (rnd_conf.editor.auto_place) {
		if ((plc[2] > 0) && (plc[3] > 0) && (plc[0] >= 0) && (plc[1] >= 0)) {
			XMoveResizeWindow(dsp, w, plc[0], plc[1], plc[2], plc[3]);
		}
		else {
			if ((plc[2] > 0) && (plc[3] > 0))
				XResizeWindow(dsp, w, plc[2], plc[3]);
			if ((plc[0] >= 0) && (plc[1] >= 0))
				XMoveWindow(dsp, w, plc[0], plc[1]);
		}
	}
	else if ((defx > 0) && (defy > 0))
		XResizeWindow(dsp, w, defx, defy);
}

static void get_win_xy(Display *dsp, Window win, int *x, int *y)
{
	Window rw, cw;

	rw = DefaultRootWindow(dsp);
	*x = *y = -1;
	XTranslateCoordinates(dsp, win, rw, 0, 0, x, y, &cw);
}


static void ltf_winplace_cfg(Display *dsp, Window win, void *ctx, const char *id)
{
	
	Window rw;
	int x = 1, y = 1, x2, y2, tmp;
	unsigned int w, h, brd, depth;
	int plc[4] = {-1, -1, -1, -1};

	get_win_xy(dsp, win, &x, &y);

	switch(wplc_hack) {
		case 0: /* place the window on the "same spot" just to see where it ends up */
			wplc_hack = 1;
			wplc_hackx = x;
			wplc_hacky = y;
			XMoveWindow(dsp, win, x, y);
			return;
		case 1: /* learn where the "same spot" placement ended up and calculate a delta */
			wplc_hack = 2;
			x2 = wplc_hackx;
			y2 = wplc_hacky;

			wplc_hackx = x - wplc_hackx;
			wplc_hacky = y - wplc_hacky;

			/* restore the original window position (either what the code wants or
			   what we learned from the GUI */
			rnd_event(ltf_hidlib, RND_EVENT_DAD_NEW_DIALOG, "psp", NULL, id, plc);
			if ((plc[0] >= 0) && (plc[1] >= 0)) {
				x = plc[0];
				y = plc[1];
			}
			else {
				x = x2 - wplc_hackx;
				y = y2 - wplc_hacky;
			}
			XMoveWindow(dsp, win, x, y);
			return;
		default: break;
	}

	
	if (wplc_hack == 2) { /* normal placement - we already know the offsets */
		XGetGeometry(dsp, win, &rw, &tmp, &tmp, &w, &h, &brd, &depth);
		x -= wplc_hackx;
		y -= wplc_hacky;
		rnd_event(ltf_hidlib, RND_EVENT_DAD_NEW_GEO, "psiiii", ctx, id, (int)x, (int)y, (int)w, (int)h);
	}
}


void pcb_ltf_wplc_config_cb(Widget shell, XtPointer data, XEvent *xevent, char *dummy)
{
	char *id = data;
	Display *dsp;
	Window win;
	XConfigureEvent *cevent = (XConfigureEvent *)xevent;

	if (cevent->type != ConfigureNotify)
		return;

	win = XtWindow(shell);
	dsp = XtDisplay(shell);
	ltf_winplace_cfg(dsp, win, NULL, id);
}


/* ------------------------------------------------------------ */

int pcb_ltf_wait_for_dialog_noclose(Widget w)
{
	pcb_ltf_ok = -1;
	XtManageChild(w);
	for(;;) {
		XEvent e;

		if (pcb_ltf_ok != -1)
			break;
		if (!XtIsManaged(w))
			break;
		XtAppNextEvent(app_context, &e);
		XtDispatchEvent(&e);
	}
	return pcb_ltf_ok;
}

int pcb_ltf_wait_for_dialog(Widget w)
{
	pcb_ltf_wait_for_dialog_noclose(w);
	if ((pcb_ltf_ok != DAD_CLOSED) && (XtIsManaged(w)))
		XtUnmanageChild(w);
	return pcb_ltf_ok;
}

/* ------------------------------------------------------------ */

typedef struct {
	void *caller_data; /* WARNING: for now, this must be the first field (see core spinbox enter_cb) */
	rnd_hid_attribute_t *attrs;
	int n_attrs;
	Widget *wl;   /* content widget */
	Widget *wltop;/* the parent widget, which is different from wl if reparenting (extra boxes, e.g. for framing or scrolling) was needed */
	Widget **btn; /* enum value buttons */
	Widget dialog;
	rnd_hid_attr_val_t property[RND_HATP_max];
	Dimension minw, minh;
	void (*close_cb)(void *caller_data, rnd_hid_attr_ev_t ev);
	char *id;
	unsigned close_cb_called:1;
	unsigned already_closing:1;
	unsigned inhibit_valchg:1;
	unsigned widget_destroyed:1;
	unsigned set_ok:1;
} lesstif_attr_dlg_t;

static void attribute_dialog_readres(lesstif_attr_dlg_t *ctx, int widx)
{
	switch(ctx->attrs[widx].type) {
		case RND_HATT_BOOL:
			ctx->attrs[widx].val.lng = XmToggleButtonGetState(ctx->wl[widx]);
			break;
		case RND_HATT_STRING:
			free((char *)ctx->attrs[widx].val.str);
			ctx->attrs[widx].val.str = rnd_strdup(XmTextGetString(ctx->wl[widx]));
			return; /* can't rely on central copy because of the allocation */
		case RND_HATT_ENUM:
			{
				const char **uptr;
				Widget btn;

				stdarg_n = 0;
				stdarg(XmNmenuHistory, &btn);
				XtGetValues(ctx->wl[widx], stdarg_args, stdarg_n);
				stdarg_n = 0;
				stdarg(XmNuserData, &uptr);
				XtGetValues(btn, stdarg_args, stdarg_n);
				ctx->attrs[widx].val.lng = uptr - (const char **)ctx->attrs[widx].wdata;
			}
			break;
		default:
			break;
	}
}

static int attr_get_idx(XtPointer dlg_widget_, lesstif_attr_dlg_t **ctx_out)
{
	lesstif_attr_dlg_t *ctx;
	Widget dlg_widget = (Widget)dlg_widget_; /* ctx->wl[i] */
	int widx;

	if (dlg_widget == NULL)
		return -1;

	XtVaGetValues(dlg_widget, XmNuserData, &ctx, NULL);

	if (ctx == NULL) {
		*ctx_out = NULL;
		return -1;
	}
	*ctx_out = ctx;

	if (ctx->inhibit_valchg)
		return -1;

	for(widx = 0; widx < ctx->n_attrs; widx++)
		if (ctx->wl[widx] == dlg_widget)
			break;

	if (widx >= ctx->n_attrs)
		return -1;

	return widx;
}

static void valchg(Widget w, XtPointer dlg_widget_, XtPointer call_data)
{
	lesstif_attr_dlg_t *ctx;
	int widx = attr_get_idx(dlg_widget_, &ctx);
	if (widx < 0)
		return;

	ctx->attrs[widx].changed = 1;

	attribute_dialog_readres(ctx, widx);

	if ((ctx->attrs[widx].change_cb == NULL) && (ctx->property[RND_HATP_GLOBAL_CALLBACK].func == NULL))
		return;

	if (ctx->property[RND_HATP_GLOBAL_CALLBACK].func != NULL)
		ctx->property[RND_HATP_GLOBAL_CALLBACK].func(ctx, ctx->caller_data, &ctx->attrs[widx]);
	if (ctx->attrs[widx].change_cb != NULL)
		ctx->attrs[widx].change_cb(ctx, ctx->caller_data, &ctx->attrs[widx]);
}

static void activated(Widget w, XtPointer dlg_widget_, XtPointer call_data)
{
	lesstif_attr_dlg_t *ctx;
	int widx = attr_get_idx(dlg_widget_, &ctx);
	if (widx < 0)
		return;

	if (ctx->attrs[widx].enter_cb != NULL)
		ctx->attrs[widx].enter_cb(ctx, ctx->caller_data, &ctx->attrs[widx]);
}

static int attribute_dialog_add(lesstif_attr_dlg_t *ctx, Widget parent, int start_from);

#include "dlg_attr_misc.c"
#include "dlg_attr_box.c"
#include "dlg_attr_tree.c"

XmString label_text_rotate(lesstif_attr_dlg_t *ctx, int i, const char *str)
{
	if ((ctx->attrs[i].rnd_hatt_flags & RND_HATF_TEXT_VERTICAL) && (str != NULL)) {
		char tmp[16];
		int nc;
		for(nc = 0; str[nc] != '\0'; nc++) {
			if (nc > 1)
				break;
			tmp[nc*2] = str[nc];
			tmp[nc*2+1] = '\n';
		}
		tmp[nc*2 + (nc > 0 ? -1 : 0)] = '\0';
		return XmStringCreatePCB(tmp);
	}
	else
		return XmStringCreatePCB(str);
}

/* returns the index of HATT_END where the loop had to stop */
static int attribute_dialog_add(lesstif_attr_dlg_t *ctx, Widget parent, int start_from)
{
	int len, i, numch, numcol;
	static XmString empty = 0;

	if (!empty)
		empty = XmStringCreatePCB(" ");

	for (i = start_from; i < ctx->n_attrs; i++) {
		Widget w;

		if (ctx->attrs[i].type == RND_HATT_END)
			break;

		/* Add content */
		stdarg_n = 0;
		stdarg(XmNalignment, XmALIGNMENT_END);
		if ((ctx->attrs[i].rnd_hatt_flags & RND_HATF_EXPFILL) || (ctx->attrs[i].type == RND_HATT_BEGIN_HPANE) || (ctx->attrs[i].type == RND_HATT_BEGIN_VPANE))
			stdarg(PxmNfillBoxFill, 1);
		stdarg(XmNuserData, ctx);

		switch(ctx->attrs[i].type) {
		case RND_HATT_BEGIN_HBOX:
			w = pcb_motif_box(parent, XmStrCast(ctx->attrs[i].name), 'h', 0, (ctx->attrs[i].rnd_hatt_flags & RND_HATF_FRAME), (ctx->attrs[i].rnd_hatt_flags & RND_HATF_SCROLL));
			XtManageChild(w);
			ctx->wltop[i] = ctx->wl[i] = w;
			i = attribute_dialog_add(ctx, w, i+1);
			break;

		case RND_HATT_BEGIN_VBOX:
			w = pcb_motif_box(parent, XmStrCast(ctx->attrs[i].name), 'v', 0, (ctx->attrs[i].rnd_hatt_flags & RND_HATF_FRAME), (ctx->attrs[i].rnd_hatt_flags & RND_HATF_SCROLL));
			XtManageChild(w);
			ctx->wltop[i] =ctx->wl[i] = w;
			i = attribute_dialog_add(ctx, w, i+1);
			break;

		case RND_HATT_BEGIN_HPANE:
		case RND_HATT_BEGIN_VPANE:
			i = ltf_pane_create(ctx, i, parent, (ctx->attrs[i].type == RND_HATT_BEGIN_HPANE));
			break;

		case RND_HATT_BEGIN_TABLE:
			/* create content table */
			numcol = ctx->attrs[i].rnd_hatt_table_cols;
			len = rnd_hid_attrdlg_num_children(ctx->attrs, i+1, ctx->n_attrs);
			numch = len  / numcol + !!(len % numcol);
			w = pcb_motif_box(parent, XmStrCast(ctx->attrs[i].name), 't', numch, (ctx->attrs[i].rnd_hatt_flags & RND_HATF_FRAME), (ctx->attrs[i].rnd_hatt_flags & RND_HATF_SCROLL));

			ctx->wl[i] = w;

			/* tables are implemented using RowColumn which works only if all rows
			   are filled in with the predefined number of widgets (numcol). If the
			   called didn't do that, the table needs to be filled up with placeholders */
			i = attribute_dialog_add(ctx, w, i+1);
			while((len % numcol) != 0) {
				Widget pad;

				stdarg_n = 0;
				stdarg(XmNalignment, XmALIGNMENT_END);
				pad = XmCreateLabel(w, XmStrCast("."), stdarg_args, stdarg_n);
				XtManageChild(pad);
				len++;
			}
			XtManageChild(w);
			break;

		case RND_HATT_BEGIN_TABBED:
			i = ltf_tabbed_create(ctx, parent, &ctx->attrs[i], i);
			break;

		case RND_HATT_BEGIN_COMPOUND:
			i = attribute_dialog_add(ctx, parent, i+1);
			break;

		case RND_HATT_PREVIEW:
			ctx->wl[i] = ltf_preview_create(ctx, parent, &ctx->attrs[i]);
			break;

		case RND_HATT_TEXT:
			ctx->wl[i] = ltf_text_create(ctx, parent, &ctx->attrs[i]);
			break;

		case RND_HATT_TREE:
TODO("The wrapper box would allow the table to shrink but then the dialog is always resized to minimum in widget update (library window)\n");
/*
			stdarg(PxmNfillBoxMinSize, 20);
			ctx->wltop[i] = pcb_motif_box(parent, "tree top", 'v', 0, 0, 0);
			ctx->wl[i] = ltf_tree_create(ctx, ctx->wltop[i], &ctx->attrs[i]);
*/
			ctx->wl[i] = ltf_tree_create(ctx, parent, &ctx->attrs[i]);
			break;

		case RND_HATT_PICTURE:
			ctx->wl[i] = ltf_picture_create(ctx, parent, &ctx->attrs[i]);
			XtAddCallback(ctx->wl[i], XmNactivateCallback, valchg, ctx->wl[i]);
			XtSetValues(ctx->wl[i], stdarg_args, stdarg_n);
			break;

		case RND_HATT_PICBUTTON:
			ctx->wl[i] = ltf_picbutton_create(ctx, parent, &ctx->attrs[i]);
			XtAddCallback(ctx->wl[i], XmNactivateCallback, valchg, ctx->wl[i]);
			XtSetValues(ctx->wl[i], stdarg_args, stdarg_n);
			break;

		case RND_HATT_COLOR:
			ctx->wl[i] = ltf_colorbtn_create(ctx, parent, &ctx->attrs[i], (ctx->attrs[i].rnd_hatt_flags & RND_HATF_CLR_STATIC));
			/* callback handled internally */
			XtSetValues(ctx->wl[i], stdarg_args, stdarg_n);
			break;
	
		case RND_HATT_LABEL:
			if (ctx->attrs[i].rnd_hatt_flags & RND_HATF_TEXT_VERTICAL)
				stdarg(XmNheight, 16);
			/* have to pretend label is a button, else it won't get the clicks */
			stdarg(XmNalignment, XmALIGNMENT_BEGINNING);
			stdarg(XmNdefaultButtonShadowThickness, 0);
			stdarg(XmNdefaultButtonEmphasis, XmEXTERNAL_HIGHLIGHT);
			stdarg(XmNshadowThickness, 0);
			stdarg(XmNshowAsDefault, 0);
			stdarg(XmNmarginBottom, 0);
			stdarg(XmNmarginTop, 0);
			stdarg(XmNmarginLeft, 0);
			stdarg(XmNmarginRight, 0);
			stdarg(XmNlabelString, label_text_rotate(ctx, i, ctx->attrs[i].name));
			ctx->wl[i] = XmCreatePushButton(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
			XtAddCallback(ctx->wl[i], XmNactivateCallback, valchg, ctx->wl[i]);
			break;
		case RND_HATT_BOOL:
			stdarg(XmNlabelString, empty);
			stdarg(XmNset, ctx->attrs[i].val.lng);
			ctx->wl[i] = XmCreateToggleButton(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
			XtAddCallback(ctx->wl[i], XmNvalueChangedCallback, valchg, ctx->wl[i]);
			break;
		case RND_HATT_STRING:
			stdarg(XmNcolumns, ((ctx->attrs[i].hatt_flags & RND_HATF_HEIGHT_CHR) ? ctx->attrs[i].geo_width : 40));
			stdarg(XmNresizeWidth, True);
			stdarg(XmNvalue, ctx->attrs[i].val.str);
			ctx->wl[i] = XmCreateTextField(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
			XtAddCallback(ctx->wl[i], XmNvalueChangedCallback, valchg, ctx->wl[i]);
			XtAddCallback(ctx->wl[i], XmNactivateCallback, activated, ctx->wl[i]);
			break;

		case RND_HATT_INTEGER:
		case RND_HATT_COORD:
		case RND_HATT_REAL:
			stdarg_n = 0;
			stdarg(XmNalignment, XmALIGNMENT_BEGINNING);
			ctx->wl[i] = XmCreateLabel(parent, XmStrCast("ERROR: old spin widget"), stdarg_args, stdarg_n);
			break;

		case RND_HATT_PROGRESS:
			ctx->wl[i] = ltf_progress_create(ctx, parent);
			break;
		case RND_HATT_ENUM:
			{
				static XmString empty = 0;
				Widget submenu, default_button = 0;
				int sn = stdarg_n;
				const char **vals = ctx->attrs[i].wdata;

				if (empty == 0)
					empty = XmStringCreatePCB("");

				submenu = XmCreatePulldownMenu(parent, XmStrCast(ctx->attrs[i].name == NULL ? "anon" : ctx->attrs[i].name), stdarg_args + sn, stdarg_n - sn);

				stdarg_n = sn;
				stdarg(XmNlabelString, empty);
				stdarg(XmNsubMenuId, submenu);
				ctx->wl[i] = XmCreateOptionMenu(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
				for (sn = 0; vals[sn]; sn++);
				ctx->btn[i] = calloc(sizeof(Widget), sn);
				for (sn = 0; vals[sn]; sn++) {
					Widget btn;
					XmString label;
					stdarg_n = 0;
					label = XmStringCreatePCB(vals[sn]);
					stdarg(XmNuserData, &vals[sn]);
					stdarg(XmNlabelString, label);
					btn = XmCreatePushButton(submenu, XmStrCast("menubutton"), stdarg_args, stdarg_n);
					XtManageChild(btn);
					XmStringFree(label);
					if (sn == ctx->attrs[i].val.lng)
						default_button = btn;
					XtAddCallback(btn, XmNactivateCallback, valchg, ctx->wl[i]);
					(ctx->btn[i])[sn] = btn;
				}
				if (default_button) {
					stdarg_n = 0;
					stdarg(XmNmenuHistory, default_button);
					XtSetValues(ctx->wl[i], stdarg_args, stdarg_n);
				}
			}
			break;
		case RND_HATT_BUTTON:
			stdarg(XmNlabelString, XmStringCreatePCB(ctx->attrs[i].val.str));
			ctx->wl[i] = XmCreatePushButton(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
			XtAddCallback(ctx->wl[i], XmNactivateCallback, valchg, ctx->wl[i]);
			break;
		default:
			ctx->wl[i] = XmCreateLabel(parent, XmStrCast("UNIMPLEMENTED"), stdarg_args, stdarg_n);
			break;
		}
		if (ctx->wl[i] != NULL)
			XtManageChild(ctx->wl[i]);
		if (ctx->wltop[i] == NULL)
			ctx->wltop[i] = ctx->wl[i];
		else
			XtManageChild(ctx->wltop[i]);
		if (ctx->attrs[i].rnd_hatt_flags & RND_HATF_INIT_FOCUS) {
			Widget w, p;
			for(w = ctx->wl[i]; w != ctx->dialog; w = p) {
				p = XtParent(w);
				XtVaSetValues(p, XmNinitialFocus, w, NULL);
			}
		}
	}
	return i;
}


static int attribute_dialog_set(lesstif_attr_dlg_t *ctx, int idx, const rnd_hid_attr_val_t *val)
{
	int save, n, copied = 0;

	save = ctx->inhibit_valchg;
	ctx->inhibit_valchg = 1;
	switch(ctx->attrs[idx].type) {
		case RND_HATT_BEGIN_HBOX:
		case RND_HATT_BEGIN_VBOX:
		case RND_HATT_BEGIN_TABLE:
		case RND_HATT_BEGIN_COMPOUND:
			goto err;
		case RND_HATT_END:
			{
				rnd_hid_compound_t *cmp = ctx->attrs[idx].wdata;
				if ((cmp != NULL) && (cmp->set_value != NULL))
					cmp->set_value(&ctx->attrs[idx], ctx, idx, val);
				else
					goto err;
			}
			break;
		case RND_HATT_BEGIN_TABBED:
			ltf_tabbed_set(ctx->wl[idx], val->lng);
			break;
		case RND_HATT_BEGIN_HPANE:
		case RND_HATT_BEGIN_VPANE:
			/* not possible to change the pane with the default motif widget */
			break;
		case RND_HATT_BUTTON:
			XtVaSetValues(ctx->wl[idx], XmNlabelString, XmStringCreatePCB(val->str), NULL);
			break;
		case RND_HATT_LABEL:
			if (ctx->attrs[idx].rnd_hatt_flags & RND_HATF_TEXT_VERTICAL)
				XtVaSetValues(ctx->wl[idx], XmNlabelString, label_text_rotate(ctx, idx, val->str), XmNheight, 16, NULL);
			else
				XtVaSetValues(ctx->wl[idx], XmNlabelString, label_text_rotate(ctx, idx, val->str), NULL);
			break;
		case RND_HATT_BOOL:
			XtVaSetValues(ctx->wl[idx], XmNset, val->lng, NULL);
			break;
		case RND_HATT_STRING:
			XtVaSetValues(ctx->wl[idx], XmNvalue, XmStrCast(val->str), NULL);
			ctx->attrs[idx].val.str = rnd_strdup(val->str);
			copied = 1;
			break;
		case RND_HATT_INTEGER:
		case RND_HATT_COORD:
		case RND_HATT_REAL:
			goto err;
		case RND_HATT_PROGRESS:
			ltf_progress_set(ctx, idx, val->dbl);
			break;
		case RND_HATT_COLOR:
			ltf_colorbtn_set(ctx, idx, &val->clr);
			break;
		case RND_HATT_PREVIEW:
			ltf_preview_set(ctx, idx, val->dbl);
			break;
		case RND_HATT_TEXT:
			ltf_text_set(ctx, idx, val->str);
			break;
		case RND_HATT_TREE:
			ltf_tree_set(ctx, idx, val->str);
			break;
		case RND_HATT_ENUM:
			{
				const char **vals = ctx->attrs[idx].wdata;
				for (n = 0; vals[n]; n++) {
					if (n == val->lng) {
						stdarg_n = 0;
						stdarg(XmNmenuHistory, (ctx->btn[idx])[n]);
						XtSetValues(ctx->wl[idx], stdarg_args, stdarg_n);
						goto ok;
					}
				}
			}
			goto err;
		default:
			goto err;
	}

	ok:;
	if (!copied)
		ctx->attrs[idx].val = *val;
	ctx->inhibit_valchg = save;
	return 0;

	err:;
	ctx->inhibit_valchg = save;
	return -1;
}

static void readres_all(lesstif_attr_dlg_t *ctx)
{
	int i;

	for (i = 0; i < ctx->n_attrs; i++) {
		attribute_dialog_readres(ctx, i);
		free(ctx->btn[i]);
	}
}

static void ltf_attr_destroy_cb(Widget w, void *v, void *cbs)
{
	lesstif_attr_dlg_t *ctx = v;

	if (ctx->set_ok)
		pcb_ltf_ok = DAD_CLOSED;

	if ((!ctx->close_cb_called) && (ctx->close_cb != NULL)) {
		ctx->close_cb_called = 1;
		ctx->close_cb(ctx->caller_data, RND_HID_ATTR_EV_WINCLOSE);
	}
	else if (!ctx->widget_destroyed) {
		ctx->widget_destroyed = 1;
		readres_all(ctx);
		XtUnmanageChild(w);
		XtDestroyWidget(w);

		if (ctx->set_ok)
			pcb_ltf_ok = DAD_CLOSED;

		if ((!ctx->close_cb_called) && (ctx->close_cb != NULL)) {
			ctx->close_cb_called = 1;
			ctx->close_cb(ctx->caller_data, RND_HID_ATTR_EV_CODECLOSE);
		}
	}
}

static void ltf_attr_config_cb(Widget shell, XtPointer data, XEvent *xevent, char *dummy)
{
	lesstif_attr_dlg_t *ctx = data;
	Display *dsp;
	XConfigureEvent *cevent = (XConfigureEvent *)xevent;

	if (cevent->type != ConfigureNotify)
		return;

	dsp = XtDisplay(shell);

	ltf_winplace_cfg(dsp, XtWindow(ctx->dialog), ctx, ctx->id);
}

static void ltf_initial_wstates(lesstif_attr_dlg_t *ctx)
{
	int n;
	for(n = 0; n < ctx->n_attrs; n++)
		if (ctx->attrs[n].rnd_hatt_flags & RND_HATF_HIDE)
			XtUnmanageChild(ctx->wltop[n]);
}

void *lesstif_attr_dlg_new(rnd_hid_t *hid, const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, rnd_hid_attr_ev_t ev), int defx, int defy, int minx, int miny)
{
	Widget topform, main_tbl;
	lesstif_attr_dlg_t *ctx;

	ctx = calloc(sizeof(lesstif_attr_dlg_t), 1);
	ctx->attrs = attrs;
	ctx->n_attrs = n_attrs;
	ctx->caller_data = caller_data;
	ctx->minw = ctx->minh = 32;
	ctx->close_cb = button_cb;
	ctx->close_cb_called = 0;
	ctx->widget_destroyed = 0;
	ctx->id = rnd_strdup(id);

	ctx->wl = (Widget *) calloc(n_attrs, sizeof(Widget));
	ctx->wltop = (Widget *)calloc(n_attrs, sizeof(Widget));
	ctx->btn = (Widget **) calloc(n_attrs, sizeof(Widget *));

	stdarg_n = 0;
	topform = XmCreateFormDialog(mainwind, XmStrCast(title), stdarg_args, stdarg_n);
	XtManageChild(topform);


	ctx->dialog = XtParent(topform);
	XtAddCallback(topform, XmNunmapCallback, ltf_attr_destroy_cb, ctx);
	XtAddEventHandler(XtParent(topform), StructureNotifyMask, False, ltf_attr_config_cb, ctx);


	stdarg_n = 0;
	stdarg(XmNfractionBase, ctx->n_attrs);
	XtSetValues(topform, stdarg_args, stdarg_n);


	if (!RND_HATT_IS_COMPOSITE(attrs[0].type)) {
		stdarg_n = 0;
		main_tbl = pcb_motif_box(topform, XmStrCast("layout"), 't', rnd_hid_attrdlg_num_children(ctx->attrs, 0, ctx->n_attrs), 0, 0);
		XtManageChild(main_tbl);
		attribute_dialog_add(ctx, main_tbl, 0);
	}
	else {
		stdarg_n = 0;
		stdarg(XmNtopAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_FORM);
		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		main_tbl = pcb_motif_box(topform, XmStrCast("layout"), 'v', 0, 0, 0);
		XtManageChild(main_tbl);
		attribute_dialog_add(ctx, main_tbl, 0);
	}

	/* don't expect screens larger than 800x600 */
	if (ctx->minw > 750)
		ctx->minw = 750;
	if (ctx->minh > 550)
		ctx->minh = 550;

	/* set top form's minimum width/height to content request */
	stdarg_n = 0;
	stdarg(XmNminWidth, ctx->minw);
	stdarg(XmNminHeight, ctx->minh);
	XtSetValues(XtParent(ctx->dialog), stdarg_args, stdarg_n);


	if (!modal)
		XtManageChild(ctx->dialog);

	XtRealizeWidget(ctx->dialog);
	pcb_ltf_winplace(XtDisplay(topform), XtWindow(XtParent(topform)), id, defx, defy);

	ltf_initial_wstates(ctx);

	return ctx;
}

void *lesstif_attr_sub_new(Widget parent_box, rnd_hid_attribute_t *attrs, int n_attrs, void *caller_data)
{
	lesstif_attr_dlg_t *ctx;

	ctx = calloc(sizeof(lesstif_attr_dlg_t), 1);
	ctx->attrs = attrs;
	ctx->n_attrs = n_attrs;
	ctx->caller_data = caller_data;

	ctx->wl = (Widget *) calloc(n_attrs, sizeof(Widget));
	ctx->wltop = (Widget *)calloc(n_attrs, sizeof(Widget));
	ctx->btn = (Widget **) calloc(n_attrs, sizeof(Widget *));

	attribute_dialog_add(ctx, parent_box, 0);
	ltf_initial_wstates(ctx);

	return ctx;
}

int lesstif_attr_dlg_run(void *hid_ctx)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;
	ctx->set_ok = 1;
	return pcb_ltf_wait_for_dialog(ctx->dialog);
}

void lesstif_attr_dlg_raise(void *hid_ctx)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;
	XRaiseWindow(XtDisplay(ctx->dialog), XtWindow(ctx->dialog));
}

void lesstif_attr_dlg_close(void *hid_ctx)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;

	if (!ctx->widget_destroyed) {
		ctx->widget_destroyed = 1;
		XtDestroyWidget(ctx->dialog);
		ltf_attr_destroy_cb(ctx->dialog, ctx, NULL);
	}
}


void lesstif_attr_dlg_free(void *hid_ctx)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;

	if (ctx->already_closing)
		return;
	ctx->already_closing = 1;

	lesstif_attr_dlg_close(ctx);

	free(ctx->wl);
	free(ctx->wltop);
	free(ctx->id);
	free(ctx);
}

void lesstif_attr_dlg_property(void *hid_ctx, rnd_hat_property_t prop, const rnd_hid_attr_val_t *val)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;
	if ((prop >= 0) && (prop < RND_HATP_max))
		ctx->property[prop] = *val;
}

int lesstif_attr_dlg_widget_state(void *hid_ctx, int idx, int enabled)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;

	if ((idx < 0) || (idx >= ctx->n_attrs) || (ctx->wl[idx] == NULL))
		return -1;

	if (ctx->attrs[idx].type == RND_HATT_BEGIN_COMPOUND)
		return -1;

	if (ctx->attrs[idx].type == RND_HATT_END) {
		rnd_hid_compound_t *cmp = ctx->attrs[idx].wdata;
		if ((cmp != NULL) && (cmp->widget_state != NULL))
			cmp->widget_state(&ctx->attrs[idx], ctx, idx, enabled);
		else
			return -1;
	}

	XtSetSensitive(ctx->wl[idx], enabled);
	return 0;
}

int lesstif_attr_dlg_widget_hide(void *hid_ctx, int idx, rnd_bool hide)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;

	if ((idx < 0) || (idx >= ctx->n_attrs) || (ctx->wl[idx] == NULL))
		return -1;

	if (ctx->attrs[idx].type == RND_HATT_BEGIN_COMPOUND)
		return -1;
	if (ctx->attrs[idx].type == RND_HATT_END) {
		rnd_hid_compound_t *cmp = ctx->attrs[idx].wdata;
		if ((cmp != NULL) && (cmp->widget_hide != NULL))
			cmp->widget_hide(&ctx->attrs[idx], ctx, idx, hide);
		else
			return -1;
	}

	if (hide)
		XtUnmanageChild(ctx->wltop[idx]);
	else
		XtManageChild(ctx->wltop[idx]);

	return 0;
}

int lesstif_attr_dlg_set_value(void *hid_ctx, int idx, const rnd_hid_attr_val_t *val)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;

	if ((idx < 0) || (idx >= ctx->n_attrs))
		return -1;

	if (attribute_dialog_set(ctx, idx, val) == 0) {
		ctx->attrs[idx].val = *val;
		return 0;
	}

	return -1;
}

void lesstif_attr_dlg_set_help(void *hid_ctx, int idx, const char *val)
{
/*	lesstif_attr_dlg_t *ctx = hid_ctx;*/
	/* lesstif doesn't have help tooltips now */
}


static const char pcb_acts_DoWindows[] = "DoWindows(1|2|3|4)\n" "DoWindows(Layout|Library|Log|Netlist)";
static const char pcb_acth_DoWindows[] = "Open various GUI windows.";
/* DOC: dowindows.html */
static fgw_error_t pcb_act_DoWindows(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *a = "";
	RND_ACT_MAY_CONVARG(1, FGW_STR, DoWindows, a = argv[1].val.str);
	if (strcmp(a, "1") == 0 || rnd_strcasecmp(a, "Layout") == 0) {
	}
	else if (strcmp(a, "2") == 0 || rnd_strcasecmp(a, "Library") == 0) {
		rnd_actionva(ltf_hidlib, "LibraryDialog", NULL);
	}
	else if (strcmp(a, "3") == 0 || rnd_strcasecmp(a, "Log") == 0) {
		rnd_actionva(ltf_hidlib, "LogDialog", NULL);
	}
	else if (strcmp(a, "4") == 0 || rnd_strcasecmp(a, "Netlist") == 0) {
		rnd_actionva(ltf_hidlib, "NetlistDialog", NULL);
	}
	else {
		RND_ACT_FAIL(DoWindows);
		RND_ACT_IRES(1);
		return 1;
	}
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_AdjustSizes[] = "AdjustSizes()";
static const char pcb_acth_AdjustSizes[] = "not supported, please use Preferences() instead";
static fgw_error_t pcb_act_AdjustSizes(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "AdjustSizes() is not supported anymore, please use the Preferences() action\n");
	RND_ACT_IRES(1);
	return 0;
}

void lesstif_update_layer_groups()
{
TODO("layer: call a redraw on the edit group")
}

rnd_hidlib_t *ltf_attr_get_dad_hidlib(void *hid_ctx)
{
	return ltf_hidlib;
}


/* ------------------------------------------------------------ */

static rnd_action_t ltf_dialog_action_list[] = {
	{"DoWindows", pcb_act_DoWindows, pcb_acth_DoWindows, pcb_acts_DoWindows},
	{"AdjustSizes", pcb_act_AdjustSizes, pcb_acth_AdjustSizes, pcb_acts_AdjustSizes}
};

void pcb_ltf_dialogs_init2(void)
{
	RND_REGISTER_ACTIONS(ltf_dialog_action_list, lesstif_cookie);
}
