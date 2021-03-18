/*
Copyright (c) 2016 Tibor 'Igor2' Palinkas
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of the Author nor the names of contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

Contact: genlist {at} igor2.repo.hu
Project page: http://repo.hu/projects/genlist
*/

#ifndef GENDLIST_H
#define GENDLIST_H
#include <stdlib.h>
#include <assert.h>

typedef struct gdl_list_s gdl_list_t;
typedef struct gdl_elem_s gdl_elem_t;

struct gdl_list_s {
	size_t length;
	void *first, *last;
	int link_offs;
};

struct gdl_elem_s {
	gdl_list_t *parent;
	void *prev, *next;
};

typedef struct gdl_iterator_s {
	size_t count;
	void *prev, *next;
} gdl_iterator_t;


#define gdl_link_offs(elem, linkfield) \
	int __link_offs__ = ((char *)(&(elem->linkfield)) - (char *)elem);

#define gdl_elem_field(list, item)  ((gdl_elem_t *)((char *)(item) + (list)->link_offs))

#define gdl_length(list)          ((list)->length)
#define gdl_first(list)           ((list)->first)
#define gdl_last(list)            ((list)->last)
#define gdl_parent(list, item)    ((item == NULL) ? NULL : gdl_elem_field(list, item)->parent)


#define gdl_next(list, item)   gdl_next_((list), (item))
#define gdl_next_(list, item)  ((item == NULL) ? NULL : gdl_elem_field(list, item)->next)

#define gdl_prev(list, item)   gdl_prev_((list), (item))
#define gdl_prev_(list, item)  ((item == NULL) ? NULL : gdl_elem_field(list, item)->prev)

#define gdl_next_nth(list, item, n, result)   gdl_next_nth_((list), (item), (n), (result))
#define gdl_next_nth_(list, item, n, result) \
do { \
	void *__i__; \
	size_t __c__ = n; \
	for(__i__ = item; (__c__) > 0 && (__i__ != NULL); __i__ = gdl_next(list, __i__)) __c__--; \
	*result = __i__; \
} while(0)

#define gdl_prev_nth(list, item, n, result)   gdl_prev_nth_((list), (item), (n), (result))
#define gdl_prev_nth_(list, item, n, result) \
do { \
	void *__i__; \
	size_t __c__ = n; \
	for(__i__ = item; (__c__) > 0 && (__i__ != NULL); __i__ = gdl_prev(list, __i__)) __c__--; \
	*result = __i__; \
} while(0)


#define gdl_append(list, elem, linkfield) gdl_append_((list), (elem), linkfield)
#define gdl_append_(list, elem, linkfield) \
do { \
	gdl_link_offs(elem, linkfield); \
	assert(elem->linkfield.parent == NULL); \
	elem->linkfield.parent = list; \
	if (list->first == NULL) { \
		assert(list->length == 0); \
		assert(list->last == NULL); \
		list->first = elem; \
		list->link_offs = __link_offs__; \
	} \
	else \
		assert(list->link_offs == __link_offs__); \
	if (list->last != NULL) { \
		gdl_elem_field(list, list->last)->next = elem; \
		elem->linkfield.prev = list->last; \
	} \
	list->last = elem; \
	elem->linkfield.next = NULL; \
	list->length++; \
} while(0)

#define gdl_insert(list, elem, linkfield) gdl_insert_((list), (elem), linkfield)
#define gdl_insert_(list, elem, linkfield) \
do { \
	gdl_link_offs(elem, linkfield); \
	assert(elem->linkfield.parent == NULL); \
	elem->linkfield.parent = list; \
	if (list->last == NULL) { \
		assert(list->length == 0); \
		assert(list->first == NULL); \
		list->last = elem; \
		list->link_offs = __link_offs__; \
	} \
	else \
		assert(list->link_offs == __link_offs__); \
	if (list->first != NULL) { \
		gdl_elem_field(list, list->first)->prev = elem; \
		elem->linkfield.next = list->first; \
	} \
	list->first = elem; \
	elem->linkfield.prev = NULL; \
	list->length++; \
} while(0)

#define gdl_insert_before(list, at_elem, elem, linkfield) gdl_insert_before_((list), (at_elem), (elem), linkfield)
#define gdl_insert_before_(list, at_elem, elem, linkfield) \
do { \
	void *__prev__; \
	gdl_link_offs(elem, linkfield); \
	assert(elem->linkfield.parent == NULL); \
	if ((at_elem == NULL) || (gdl_elem_field(list, at_elem)->prev == NULL)) { \
		gdl_insert_(list, elem, linkfield); \
		elem->linkfield.parent = list; \
		break; \
	} \
	elem->linkfield.parent = list; \
	assert(list->link_offs == __link_offs__); \
	__prev__ = gdl_elem_field(list, at_elem)->prev; \
	gdl_elem_field(list, __prev__)->next = elem; \
	gdl_elem_field(list, at_elem)->prev = elem; \
	elem->linkfield.prev = __prev__; \
	elem->linkfield.next = at_elem; \
	list->length++; \
} while(0)

#define gdl_insert_after(list, at_elem, elem, linkfield) gdl_insert_after_((list), (at_elem), (elem), linkfield)
#define gdl_insert_after_(list, at_elem, elem, linkfield) \
do { \
	void *__next__; \
	gdl_link_offs(elem, linkfield); \
	assert(elem->linkfield.parent == NULL); \
	if ((at_elem == NULL) || (gdl_elem_field(list, at_elem)->next == NULL)) { \
		gdl_append_(list, elem, linkfield); \
		break; \
	} \
	elem->linkfield.parent = list; \
	assert(list->link_offs == __link_offs__); \
	__next__ = gdl_elem_field(list, at_elem)->next; \
	gdl_elem_field(list, __next__)->prev = elem; \
	gdl_elem_field(list, at_elem)->next = elem; \
	elem->linkfield.prev = at_elem; \
	elem->linkfield.next = __next__; \
	list->length++; \
} while(0)

#define gdl_remove(list, elem, linkfield) gdl_remove_((list), (elem), linkfield)
#define gdl_remove_(list, elem, linkfield) \
do { \
	void *__next__; \
	void *__prev__; \
	gdl_link_offs(elem, linkfield); \
	assert(elem->linkfield.parent == list); \
	assert(list->link_offs == __link_offs__); \
	__prev__ = gdl_elem_field(list, elem)->prev; \
	__next__ = gdl_elem_field(list, elem)->next; \
	if (__prev__ != NULL) { \
		assert(list->first != elem); \
		gdl_elem_field(list, __prev__)->next = __next__; \
	} \
	else { \
		assert(list->first == elem); \
		list->first = __next__;  \
	} \
	if (__next__ != NULL) { \
		assert(list->last != elem); \
		gdl_elem_field(list, __next__)->prev = __prev__; \
	} \
	else { \
		assert(list->last == elem); \
		list->last = __prev__;  \
	} \
	list->length--; \
	gdl_elem_field(list, elem)->prev = NULL; \
	gdl_elem_field(list, elem)->next = NULL; \
	gdl_elem_field(list, elem)->parent = NULL; \
} while(0)

#define gdl_swap(list, elema, elemb, linkfield) gdl_swap_((list), (elema), (elemb), linkfield)
#define gdl_swap_(list, elema, elemb, linkfield) \
do { \
	void *__tmp__, *__prev__, *__next__; \
	gdl_link_offs(elema, linkfield); \
	assert(elema->linkfield.parent == list); \
	assert(elemb->linkfield.parent == list); \
	assert(list->link_offs == __link_offs__); \
	if (elema == elemb) \
		break; \
	if (list->first == elema) \
		list->first = elemb; \
	else if (list->first == elemb) \
		list->first = elema; \
	if (list->last == elema) \
		list->last = elemb; \
	else if (list->last == elemb) \
		list->last = elema; \
	if (gdl_elem_field(list, elema)->next == elemb) { \
		gdl_swap_neigh_(list, elema, elemb, linkfield); \
		break; \
	} \
	if (gdl_elem_field(list, elemb)->next == elema) { \
		gdl_swap_neigh_(list, elemb, elema, linkfield); \
		break; \
	} \
	__prev__ = gdl_elem_field(list, elema)->prev; \
	if (__prev__ != NULL) \
		gdl_elem_field(list, __prev__)->next = elemb; \
	__prev__ = gdl_elem_field(list, elemb)->prev; \
	if (__prev__ != NULL) \
		gdl_elem_field(list, __prev__)->next = elema; \
	__next__ = gdl_elem_field(list, elema)->next; \
	if (__next__ != NULL)\
		gdl_elem_field(list, __next__)->prev = elemb; \
	__next__ = gdl_elem_field(list, elemb)->next; \
	if (__next__ != NULL) \
		gdl_elem_field(list, __next__)->prev = elema; \
	__tmp__ = gdl_elem_field(list, elema)->prev; \
	gdl_elem_field(list, elema)->prev = gdl_elem_field(list, elemb)->prev; \
	gdl_elem_field(list, elemb)->prev = __tmp__; \
	__tmp__ = gdl_elem_field(list, elema)->next; \
	gdl_elem_field(list, elema)->next = gdl_elem_field(list, elemb)->next; \
	gdl_elem_field(list, elemb)->next = __tmp__; \
} while(0)

/* internal: swap a,b if they are neighbours, in this order */
#define gdl_swap_neigh_(list, elema, elemb, linkfield) \
do { \
	void *__prev__, *__next__; \
	__prev__ = gdl_elem_field(list, elema)->prev; \
	__next__ = gdl_elem_field(list, elemb)->next; \
	gdl_elem_field(list, elemb)->prev = __prev__; \
	gdl_elem_field(list, elemb)->next = elema; \
	gdl_elem_field(list, elema)->prev = elemb; \
	gdl_elem_field(list, elema)->next = __next__; \
	if (__prev__ != NULL) \
		gdl_elem_field(list, __prev__)->next = elemb; \
	if (__next__ != NULL) \
		gdl_elem_field(list, __next__)->prev = elema; \
} while(0)


#define gdl_reverse(list) gdl_reverse_((list))
#define gdl_reverse_(list) \
do { \
	void *__i__, *__tmp__, *__next__; \
	for(__i__ = list->first;  __i__ != NULL; __i__ = __next__) {\
		__next__ = gdl_next(list, __i__); \
		__tmp__ = gdl_elem_field(list, __i__)->prev; \
		gdl_elem_field(list, __i__)->prev = gdl_elem_field(list, __i__)->next; \
		gdl_elem_field(list, __i__)->next = __tmp__; \
	} \
	__tmp__ = list->first; \
	list->first = list->last; \
	list->last = __tmp__; \
} while(0)

#define gdl_foreach(list, iterator, loop_elem) gdl_foreach_((list), (iterator), (loop_elem))
#define gdl_foreach_(list, iterator, loop_elem) \
	for( \
		loop_elem = list->first, iterator->prev = NULL, iterator->next = gdl_next(list, loop_elem), iterator->count = 0; \
		loop_elem != NULL; \
		loop_elem = iterator->next, iterator->next = gdl_next(list, iterator->next), iterator->count++ \
	)

#define gdl_it_idx(iterator) ((iterator)->count)

#define gdl_nth(list, index, res_elem)  gdl_nth_((list), (index), (res_elem))
#define gdl_nth_(list, index, res_elem) \
do { \
	size_t __n__; \
	void *__res__; \
	for(__n__ = 0, __res__ = list->first; __n__ < index; __n__++, __res__=gdl_next(list, __res__)) ; \
	*res_elem = __res__; \
} while(0)


#define gdl_index(list, elem, linkfield, result) gdl_index_((list), (elem), linkfield, (result))
#define gdl_index_(list, elem, linkfield, result) \
do { \
	size_t __count__ = 0; \
	void *__i__; \
	gdl_link_offs(elem, linkfield); \
	assert(elem->linkfield.parent == list); \
	assert(list->link_offs == __link_offs__); \
	for(__i__ = elem; __i__ != NULL; __i__ = gdl_prev(list, __i__)) __count__++; \
	*(result) = __count__ - 1;  \
} while(0)


#define gdl_check(list) gdl_check_((list))
#define gdl_check_(list) \
do { \
	void *__prev__, *__i__; \
	size_t __count__; \
	for( \
		__i__ = list->first, __count__ = 0, __prev__ = 0; \
		__i__ != NULL; \
		__prev__ = __i__, __i__ = gdl_next(list, __i__), __count__++ \
	) {\
		assert(gdl_prev(list, __i__) == __prev__); \
	} \
	assert(list->length == __count__); \
	assert(list->last == __prev__); \
} while(0)


#define gdl_find(list, data, compar, result) gdl_find_((list), (data), (compar), (result))
#define gdl_find_(list, data, compar, result) \
do { \
	void *__i__, *__r__ = NULL; \
	for(__i__ = list->first; __i__ != NULL; __i__ = gdl_next(list, __i__)) { \
		if (compar(__i__, data) == 0) { \
			__r__ = __i__; \
			break; \
		} \
	} \
	*result = __r__; \
} while(0);

#define gdl_find_next(list, elem, data, compar, result) gdl_find_next_((list), (elem), (data), (compar), (result))
#define gdl_find_next_(list, elem, data, compar, result) \
do { \
	void *__i__, *__r__ = NULL; \
	assert(gdl_elem_field(list, elem)->parent == list); \
	for(__i__ = gdl_next(list, elem); __i__ != NULL; __i__ = gdl_next(list, __i__)) { \
		if (compar(__i__, data) == 0) { \
			__r__ = __i__; \
			break; \
		} \
	} \
	*result = __r__; \
} while(0);

#define gdl_ctxfind(list, data, compar, ctx, result) gdl_ctxfind_((list), (data), (compar), (ctx), (result))
#define gdl_ctxfind_(list, data, compar, ctx, result) \
do { \
	void *__i__, *__r__ = NULL; \
	for(__i__ = list->first; __i__ != NULL; __i__ = gdl_next(list, __i__)) { \
		if (compar(ctx, __i__, data) == 0) { \
			__r__ = __i__; \
			break; \
		} \
	} \
	*result = __r__; \
} while(0);

#define gdl_ctxfind_next(list, elem, data, compar, ctx, result) gdl_ctxfind_next_((list), (elem), (data), (compar), (ctx), (result))
#define gdl_ctxfind_next_(list, elem, data, compar, ctx, result) \
do { \
	void *__i__, *__r__ = NULL; \
	assert(gdl_elem_field(list, elem)->parent == list); \
	for(__i__ = gdl_next(list, elem); __i__ != NULL; __i__ = gdl_next(list, __i__)) { \
		if (compar(ctx, __i__, data) == 0) { \
			__r__ = __i__; \
			break; \
		} \
	} \
	*result = __r__; \
} while(0);

#define gdl_pos(list, data, result) gdl_find_((list), (data), (result))
#define gdl_pos_(list, data, result) \
do { \
	void *__i__, *__r__ = NULL; \
	result = 0; \
	for(__i__ = list->first; __i__ != NULL; __i__ = gdl_next(list, __i__), result++) \
		if (__i__ == data) \
			break; \
	if (__i__ != data) \
		result = -1; \
} while(0);

#endif
