/*
    fungw - language-agnostic function gateway
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

    Project page: http://repo.hu/projects/fungw
    Version control: svn://repo.hu/fungw/trunk
*/

/* The "C engine": helpers for functions implemented in C */

#include <libfungw/fungw.h>
#include <stdlib.h>

fgw_error_t fgws_c_call_script(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	fgw_error_t rv;
	fgw_func_t *fnc = argv[0].val.func;
	rv = fnc->func(res, argc, argv);
	fgw_argv_free(fnc->obj->parent, argc, argv);
	return rv;
}


fgw_eng_t fgw_c_eng = {
	"c",
	fgws_c_call_script,
	NULL, NULL, NULL, NULL, NULL, NULL, /* these shall be overwritten by the host app */
	".so"
};
