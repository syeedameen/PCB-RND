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

/* Helper macros to convert types, per class, in a switch */

#define conv_assign(dst, src) dst = (src)
#define conv_rev_assign(dst, src) src = (dst)
#define conv_round(dst, src) dst = fungw_round(src)
#define conv_err(dst, src) return -1
#define conv_break(dst, src) break
#define conv_strtol(dst, src) \
	do { \
		char *end; \
		if (src == NULL) \
			return -1; \
		if ((src[0] == '0') && (src[1] == 'x')) \
			dst = strtol(src+2, &end, 16); \
		else \
			dst = strtol(src, &end, 10); \
		if (*end != '\0') \
			return -1; \
	} while(0)
#define conv_strtod(dst, src) \
	do { \
		char *end; \
		if (src == NULL) \
			return -1; \
		dst = strtod(src, &end); \
		if (*end != '\0') \
			return -1; \
	} while(0)
#ifdef FUNGW_CFG_LONG
#define conv_strtoll(dst, src) \
	do { \
		char *end; \
		if (src == NULL) \
			return -1; \
		if ((src[0] == '0') && (src[1] == 'x')) \
			dst = strtoll(src+2, &end, 16); \
		else \
			dst = strtoll(src, &end, 10); \
		if (*end != '\0') \
			return -1; \
	} while(0)
#define conv_strtold(dst, src) \
	do { \
		char *end; \
		if (src == NULL) \
			return -1; \
		dst = strtold(src, &end); \
		if (*end != '\0') \
			return -1; \
	} while(0)
#endif

#define ARG_CONV_CASE_LONG(dst, func) \
		case FGW_CHAR:   func(dst, arg->val.nat_char); break; \
		case FGW_UCHAR:  func(dst, arg->val.nat_uchar); break; \
		case FGW_SCHAR:  func(dst, arg->val.nat_schar); break; \
		case FGW_SHORT:  func(dst, arg->val.nat_short); break; \
		case FGW_USHORT: func(dst, arg->val.nat_ushort); break; \
		case FGW_INT:    func(dst, arg->val.nat_int); break; \
		case FGW_UINT:   func(dst, arg->val.nat_uint); break; \
		case FGW_LONG:   func(dst, arg->val.nat_long); break; \
		case FGW_ULONG:  func(dst, arg->val.nat_ulong); break; \

#define ARG_CONV_CASE_DOUBLE(dst, func) \
		case FGW_FLOAT:  func(dst, arg->val.nat_float); break; \
		case FGW_DOUBLE: func(dst, arg->val.nat_double); break; \

#ifdef FUNGW_CFG_LONG
#	define ARG_CONV_CASE_LDOUBLE(dst, func) \
		case FGW_LDOUBLE: func(dst, arg->val.nat_double); break;
#	define ARG_CONV_CASE_LLONG(dst, func) \
		case FGW_LLONG:  func(dst, arg->val.nat_llong); break; \
		case FGW_ULLONG: func(dst, arg->val.nat_ullong); break; \
		case FGW_SIZE_T: func(dst, arg->val.nat_size_t); break;
#else
#	define ARG_CONV_CASE_LDOUBLE(dst, func)
#	define ARG_CONV_CASE_LLONG(dst, func) \
		case FGW_SIZE_T: func(dst, arg->val.nat_size_t); break;
#endif

#define ARG_CONV_CASE_PTR(dst, func) \
		case FGW_STRUCT: func(dst, arg->val.ptr_void); break; \
		case FGW_VOID: func(dst, arg->val.ptr_void); break; \

#define ARG_CONV_CASE_STR(dst, func) \
		case FGW_STR: { \
			char *__str__ = arg->val.ptr_char; \
			int __need2free__ = arg->type & FGW_DYN; \
			func(dst, __str__); \
			if (__need2free__) \
				free(__str__); \
			break; \
		}

#define ARG_CONV_CASE_CLASS(dst, func) \
		case FGW_PTR:   func(dst, 0); break; \
		case FGW_ZTERM: func(dst, 0); break; \

#define ARG_CONV_CASE_INVALID(dst, func) \
		case FGW_INVALID: func(dst, 0); break; \
		case FGW_FUNC: func(dst, 0); break; \
		case FGW_DYN: func(dst, 0); break; \
		case FGW_CUSTOM: func(dst, 0); break;

double fungw_round(double x);
