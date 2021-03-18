/*
    fungw - language-agnostic function gateway
    Copyright (C) 2017, 2018  Tibor 'Igor2' Palinkas

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

/* In-place converter of fgw_arg_t from one type to another */

#include <math.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "fungw.h"
#include "fungw_conv.h"

/* round() is not c89 and glibc requires feature macros for it. To avoid
   all the risk, just implement a local inlinable function that depends only
   on ceil. */
double fungw_round(double x)
{
	double t;
	if (x >= 0.0) {
		t = ceil(x);
		if (t - x > 0.5)
			t -= 1.0;
		return t;
	}
	t = ceil(-x);
	if ((t + x) > 0.5)
		t -= 1.0;
	return -t;
}

void fgw_arg_free_str(fgw_ctx_t *ctx, fgw_arg_t *arg)
{
	free(arg->val.str);
	arg->type = FGW_INVALID;
}

void fgw_arg_free(fgw_ctx_t *ctx, fgw_arg_t *arg)
{
	int baset;

	if (!(arg->type & FGW_DYN)) {
		arg->type = FGW_INVALID;
		return;
	}
	if ((arg->type & FGW_CHAR) && (arg->type & FGW_PTR)) {
		arg->type = FGW_INVALID;
		free(arg->val.str);
		return;
	}

	if (ctx->custype == NULL) {
		arg->type = FGW_INVALID;
		return;
	}
	baset = FGW_BASE_TYPE(arg->type);
	if ((baset >= FGW_CUSTOM) && (baset < FGW_CUSTOM + FGW_NUM_CUSTOM_TYPES)) {
		if ((ctx->custype[baset - FGW_CUSTOM].name == NULL) || (ctx->custype[baset - FGW_CUSTOM].arg_free == NULL)) {
			arg->type = FGW_INVALID;
			return;
		}
		ctx->custype[baset - FGW_CUSTOM].arg_free(ctx, arg);
	}
	arg->type = FGW_INVALID;
}

void fgw_argv_free(fgw_ctx_t *ctx, int argc, fgw_arg_t *argv)
{
	for(; argc > 0; argc--,argv++)
		fgw_arg_free(ctx, argv);
}

int fgw_arg_conv_to_long(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	unsigned long tmp;
	switch(FGW_BASE_TYPE(arg->type)) {
		ARG_CONV_CASE_LONG(tmp, conv_assign)
		ARG_CONV_CASE_LLONG(tmp, conv_assign)
		ARG_CONV_CASE_DOUBLE(tmp, conv_round)
		ARG_CONV_CASE_LDOUBLE(tmp, conv_round)
		ARG_CONV_CASE_STR(tmp, conv_strtol)
		ARG_CONV_CASE_PTR(tmp, conv_err)
		ARG_CONV_CASE_CLASS(tmp, conv_err)
		case FGW_AUTO:
		ARG_CONV_CASE_INVALID(tmp, conv_err)
	}
	switch(target) {
		ARG_CONV_CASE_LONG(tmp, conv_rev_assign)
		ARG_CONV_CASE_LLONG(tmp, conv_err)
		ARG_CONV_CASE_DOUBLE(tmp, conv_err)
		ARG_CONV_CASE_LDOUBLE(tmp, conv_err)
		ARG_CONV_CASE_PTR(tmp, conv_err)
		ARG_CONV_CASE_STR(tmp, conv_err)
		ARG_CONV_CASE_CLASS(tmp, conv_err)
		case FGW_AUTO:
		ARG_CONV_CASE_INVALID(tmp, conv_err)
	}
	arg->type = target;
	return 0;
}

int fgw_arg_conv_to_llong(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
#ifdef FUNGW_CFG_LONG
	unsigned long long tmp;
#else
	unsigned long tmp;
#endif
	switch(FGW_BASE_TYPE(arg->type)) {
		ARG_CONV_CASE_LONG(tmp, conv_assign)
		ARG_CONV_CASE_LLONG(tmp, conv_assign)
		ARG_CONV_CASE_DOUBLE(tmp, conv_round)
		ARG_CONV_CASE_LDOUBLE(tmp, conv_round)
#ifdef FUNGW_CFG_LONG
		ARG_CONV_CASE_STR(tmp, conv_strtoll)
#else
		ARG_CONV_CASE_STR(tmp, conv_strtol)
#endif
		ARG_CONV_CASE_PTR(tmp, conv_err)
		ARG_CONV_CASE_CLASS(tmp, conv_err)
		case FGW_AUTO:
		ARG_CONV_CASE_INVALID(tmp, conv_err)
	}
	switch(target) {
		ARG_CONV_CASE_LONG(tmp, conv_err)
		ARG_CONV_CASE_LLONG(tmp, conv_rev_assign)
		ARG_CONV_CASE_DOUBLE(tmp, conv_err)
		ARG_CONV_CASE_LDOUBLE(tmp, conv_err)
		ARG_CONV_CASE_PTR(tmp, conv_err)
		ARG_CONV_CASE_STR(tmp, conv_err)
		ARG_CONV_CASE_CLASS(tmp, conv_err)
		case FGW_AUTO:
		ARG_CONV_CASE_INVALID(tmp, conv_err)
	}
	arg->type = target;
	return 0;
}

int fgw_arg_conv_to_double(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	double tmp;
	switch(FGW_BASE_TYPE(arg->type)) {
		ARG_CONV_CASE_LONG(tmp, conv_assign)
		ARG_CONV_CASE_LLONG(tmp, conv_assign)
		ARG_CONV_CASE_DOUBLE(tmp, conv_assign)
		ARG_CONV_CASE_LDOUBLE(tmp, conv_assign)
		ARG_CONV_CASE_STR(tmp, conv_strtod)
		ARG_CONV_CASE_PTR(tmp, conv_err)
		ARG_CONV_CASE_CLASS(tmp, conv_err)
		case FGW_AUTO:
		ARG_CONV_CASE_INVALID(tmp, conv_err)
	}
	switch(target) {
		ARG_CONV_CASE_LONG(tmp, conv_err)
		ARG_CONV_CASE_LLONG(tmp, conv_err)
		ARG_CONV_CASE_DOUBLE(tmp, conv_rev_assign)
		ARG_CONV_CASE_LDOUBLE(tmp, conv_err)
		ARG_CONV_CASE_PTR(tmp, conv_err)
		ARG_CONV_CASE_STR(tmp, conv_err)
		ARG_CONV_CASE_CLASS(tmp, conv_err)
		case FGW_AUTO:
		ARG_CONV_CASE_INVALID(tmp, conv_err)
	}
	arg->type = target;
	return 0;
}

int fgw_arg_conv_to_ldouble(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
#ifdef FUNGW_CFG_LONG
	long double tmp;
#else
	double tmp;
#endif
	switch(FGW_BASE_TYPE(arg->type)) {
		ARG_CONV_CASE_LONG(tmp, conv_assign)
		ARG_CONV_CASE_LLONG(tmp, conv_assign)
		ARG_CONV_CASE_DOUBLE(tmp, conv_assign)
		ARG_CONV_CASE_LDOUBLE(tmp, conv_assign)
#ifdef FUNGW_CFG_LONG
		ARG_CONV_CASE_STR(tmp, conv_strtold)
#else
		ARG_CONV_CASE_STR(tmp, conv_strtod)
#endif
		ARG_CONV_CASE_PTR(tmp, conv_err)
		ARG_CONV_CASE_CLASS(tmp, conv_err)
		case FGW_AUTO:
		ARG_CONV_CASE_INVALID(tmp, conv_err)
	}
	switch(target) {
		ARG_CONV_CASE_LONG(tmp, conv_err)
		ARG_CONV_CASE_LLONG(tmp, conv_err)
		ARG_CONV_CASE_DOUBLE(tmp, conv_err)
		ARG_CONV_CASE_LDOUBLE(tmp, conv_rev_assign)
		ARG_CONV_CASE_PTR(tmp, conv_err)
		ARG_CONV_CASE_STR(tmp, conv_err)
		ARG_CONV_CASE_CLASS(tmp, conv_err)
		case FGW_AUTO:
		ARG_CONV_CASE_INVALID(tmp, conv_err)
	}
	(void)tmp;
	arg->type = target;
	return 0;
}

int fgw_arg_conv_to_str(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	char *s;

	if (FGW_BASE_TYPE(arg->type) == FGW_STR)  {
		if (!(target & FGW_DYN)) /* want static string, have static or dynamic string */
			return 0;
		s = fgw_strdup(arg->val.str);
		goto ok; /* have static string, want dynamic */
	}

	s = malloc(128);
	if (arg->type & FGW_PTR) {
		if (arg->val.ptr_void == NULL)
			*s = '\0';
		else
			sprintf(s, "%p", arg->val.ptr_void);
		goto ok;
	}
	switch(FGW_BASE_TYPE(arg->type)) {
		case FGW_STR:     break;
		case FGW_CHAR:    sprintf(s, "%c", arg->val.nat_char); goto ok;
		case FGW_UCHAR:   sprintf(s, "%u", arg->val.nat_uchar); goto ok;
		case FGW_SCHAR:   sprintf(s, "%d", arg->val.nat_schar); goto ok;
		case FGW_SHORT:   sprintf(s, "%d", arg->val.nat_short); goto ok;
		case FGW_USHORT:  sprintf(s, "%u", arg->val.nat_ushort); goto ok;
		case FGW_INT:     sprintf(s, "%d", arg->val.nat_int); goto ok;
		case FGW_UINT:    sprintf(s, "%u", arg->val.nat_uint); goto ok;
		case FGW_LONG:    sprintf(s, "%ld", arg->val.nat_long); goto ok;
		case FGW_ULONG:   sprintf(s, "%lu", arg->val.nat_ulong); goto ok;
#ifdef FUNGW_CFG_LONG
		case FGW_SIZE_T:  sprintf(s, "%zd", arg->val.nat_size_t); goto ok;
#else
		case FGW_SIZE_T:  sprintf(s, "%ld", (long)arg->val.nat_size_t); goto ok;
#endif
		case FGW_FLOAT:   sprintf(s, "%f", arg->val.nat_float); goto ok;
		case FGW_DOUBLE:  sprintf(s, "%f", arg->val.nat_double); goto ok;
#ifdef FUNGW_CFG_LONG
		case FGW_LDOUBLE: sprintf(s, "%d", arg->val.nat_short); goto ok;
		case FGW_LLONG:   sprintf(s, "%lld", arg->val.nat_llong); goto ok;
		case FGW_ULLONG:  sprintf(s, "%llu", arg->val.nat_ullong); goto ok;
#endif
		ARG_CONV_CASE_PTR(arg, conv_break)
		case FGW_AUTO:
		ARG_CONV_CASE_INVALID(arg, conv_break)
	}
	free(s);
	return -1;

	ok:;
	arg->type = FGW_STR | FGW_DYN;
	arg->val.str = s;
	return 0;
}

/* convert hex pointer string to pointer - can't use strtol(), sizeof(long) may be less than sizeof(void *) */
static ptrdiff_t strtoptr(char *s, char **end)
{
	ptrdiff_t d, v = 0;
	if ((s[0] == '0') && (s[1] == 'x'))
		s += 2;
	for(; *s != '\0'; s++) {
		if ((*s >= '0') && (*s <= '9')) d = *s - '0';
		else if ((*s >= 'a') && (*s <= 'f')) d = *s - 'a' + 10;
		else if ((*s >= 'A') && (*s <= 'F')) d = *s - 'A' + 10;
		else
			break;
		v = v << 4;
		v |= d;
	}
	if (end != NULL)
		*end = s;
	return v;
}

int fgw_arg_conv_to_ptr(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	if (FGW_BASE_TYPE(arg->type) == FGW_STR)  {
		char *end;
		ptrdiff_t l;
		l = strtoptr(arg->val.str, &end);
		if (*end != '\0')
			return -1;
		if (arg->type & FGW_DYN)
			fgw_arg_free_str(ctx, arg);

		arg->type = target;
		arg->val.ptr_void = (void *)l;
		return 0;
	}

	if (arg->type & FGW_PTR) {
		arg->type = target;
		return 0;
	}

	return -1;
}


#define custom_type_conv \
	if ((baset >= FGW_CUSTOM) && (baset < FGW_CUSTOM + FGW_NUM_CUSTOM_TYPES)) { \
		if ((ctx->custype == NULL) || (ctx->custype[baset - FGW_CUSTOM].name == NULL) || (ctx->custype[baset - FGW_CUSTOM].arg_conv == NULL)) \
			return -1; \
		if (ctx->custype[baset - FGW_CUSTOM].arg_conv(ctx, arg, target) != 0) \
			return -1; \
		if ((arg->type & 0xFF) == target) \
			return 0; \
	}
/* ^^^ fall through: the custom converter converted to some base type - proceed */


int fgw_arg_conv(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	int baset = (arg->type & 0xFF);

	if (arg->type == target)
		return 0;

	custom_type_conv; /* convert source custom type to something knonw, if needed */
	if (target == FGW_AUTO)
		return 0;

	baset = (target & 0xFF);
	custom_type_conv; /* convert to dest custom type if needed */

	if ((target & FGW_STR) == FGW_STR)
		return fgw_arg_conv_to_str(ctx, arg, target);

	if (target & FGW_PTR)
		return fgw_arg_conv_to_ptr(ctx, arg, target);

	if ((target & 0xFF) < 0x30)
		return fgw_arg_conv_to_long(ctx, arg, target);

	if ((target & 0xFF) < 0x40)
		return fgw_arg_conv_to_llong(ctx, arg, target);

	if ((target & 0xFF) < 0x50)
		return fgw_arg_conv_to_double(ctx, arg, target);

	if ((target & 0xFF) < 0x60)
		return fgw_arg_conv_to_ldouble(ctx, arg, target);


	return -1;
}
