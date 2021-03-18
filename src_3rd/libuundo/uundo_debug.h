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

#ifndef UUNDO_DEBUG_H
#define UUNDO_DEBUG_H

#include <stdio.h>
#include "uundo.h"

void uundo_dump(uundo_list_t *lst, FILE *f, const char *prefix);

/* Runs various integrity checks on the list; returns NULL if there's no error
   or a pointer to the error message after the first error detected. The error
   message is copied into msg that is supplied by the caller (must be at least
   256 bytes long). If msg is NULL, a static buffer is used (not reentrant!). */
const char *uundo_check(const uundo_list_t *lst, char *msg);

#endif
