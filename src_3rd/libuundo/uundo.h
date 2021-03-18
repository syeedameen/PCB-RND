/*
    libuundo - micro-undo, a small, callback based undo/redo list implementation
    Copyright (C) 2017  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Author: libuundo [+at+] igor2.repo.hu
    VCS:    svn://repo.hu/libuundo/trunk
    Web:    http://repo.hu/projects/libuundo
*/

#ifndef UUNDO_H
#define UUNDO_H

#include <stddef.h>

typedef struct uundo_list_s uundo_list_t;    /* list of undoable actions */
typedef struct uundo_item_s uundo_item_t;    /* an item on the list */
typedef struct uundo_oper_s uundo_oper_t;    /* operator: user provided function pointers for handling an item */

/* serial number type */
typedef long uundo_serial_t;
#define UUNDO_SERIAL_INVALID (-1)

struct uundo_oper_s {
	const char *cookie;              /* plugin cookie; always matched as pointer, not as string */
	void (*item_free)(void *udata);  /* free fields of the undo slot */
	int (*item_undo)(void *udata);   /* perform undo (don't free slot fields); return 0 on success */
	int (*item_redo)(void *udata);   /* perform redo (don't free slot fields); return 0 on success */
	void (*item_print)(void *udata, char *dst, size_t dst_len);   /* debug: print the operator and operands in dst */
};

struct uundo_item_s {
	uundo_serial_t serial;
	const uundo_oper_t *oper;
	uundo_item_t *prev, *next;   /* doubly linked list of items */
	char udata[1];               /* "flexible-size array" (C89-style); data for the callbacks start here */
};

struct uundo_list_s {
	uundo_serial_t serial;
	uundo_serial_t saved_serial;
	size_t num_undo;    /* number of slots on the list (up to tail) */
	size_t num_redo;    /* number of redo items on the list (beyond tail) */
	uundo_item_t *head; /* oldest item */
	uundo_item_t *tail; /* newest item; an undo will undo this; the list may extend beyond this item (for redo) */
	unsigned long freeze_serial;  /* when non-zero, don't change the serial */
	unsigned long freeze_add;     /* when non-zero, do not store new items */
	uundo_item_t *frozen;         /* temporary items added under freeze_add */
};


/*** list management ***/

/* Initialize a previously uninitialized list lst; result is an empty list */
void uundo_list_init(uundo_list_t *lst);

/* Clear a previously initialized list lst (free undo slots); result is an empty list */
void uundo_list_clear(uundo_list_t *lst);

/* Clear a previously initialized list lst (free undo slots) */
void uundo_list_uninit(uundo_list_t *lst);

/* Clear the redo section */
void uundo_list_truncate_redo(uundo_list_t *lst);

/* Clear items from the sfirst serial (inclusive) to the tail of the list;
   also clears the redo section. */
void uundo_list_truncate_from(uundo_list_t *lst, uundo_serial_t sfirst);

/* Search the undo list from tail back for a cookie; returns the first serial
   that references the cookie or -1 if no item references the cookie */
uundo_serial_t uundo_list_find_cookie(uundo_list_t *lst, const char *cookie);


/*** undo/redo ***/

/* Allocate a new undo item at tail; clears the redo section; returns the new
   item's udata, allocated to be data_len bytes long */
void *uundo_append(uundo_list_t *lst, const uundo_oper_t *oper, size_t data_len);

/* Performs undo of all items with the highest serial number */
int uundo_undo(uundo_list_t *lst);

/* Performs undo of all items with serial number higher than s_min */
int uundo_undo_above(uundo_list_t *lst, uundo_serial_t s_min);

/* Performs redo of all items with the lowest redoable serial number */
int uundo_redo(uundo_list_t *lst);

/* Performs redo of all redoable items with serial number lower than s_till */
int uundo_redo_below(uundo_list_t *lst, uundo_serial_t s_till);

/*** serial management ***/

/* Save or restore serial number for bundling actions under a single serial number */
#define uundo_inc_serial(lst) \
do { \
	if (!((lst)->freeze_serial)) \
		(lst)->serial++; \
} while(0) \

#define uundo_save_serial(lst) \
do { \
	if (!((lst)->freeze_serial)) \
		(lst)->saved_serial = (lst)->serial; \
} while(0) \

#define uundo_restore_serial(lst) \
do { \
	if (!((lst)->freeze_serial)) \
		(lst)->serial = (lst)->saved_serial; \
} while(0) \

#define uundo_freeze_serial(lst) (lst)->freeze_serial++

#define uundo_unfreeze_serial(lst) \
do { \
	if (((lst)->freeze_serial) > 0) \
		(lst)->freeze_serial--; \
} while(0) \

/*** add freeze management ***/

#define uundo_freeze_add(lst) (lst)->freeze_add++

#define uundo_unfreeze_add(lst) \
do { \
	if (((lst)->freeze_add) > 0) { \
		(lst)->freeze_add--; \
		if (((lst)->freeze_add) == 0) \
			uundo_flush_frozen(lst); \
	} \
} while(0) \

/* Internal call: free all frozen temp storage records */
void uundo_flush_frozen(uundo_list_t *lst);


#endif

