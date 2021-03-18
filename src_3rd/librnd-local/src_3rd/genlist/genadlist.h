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

#ifndef GENADLIST_H
#define GENADLIST_H
#include <stddef.h>
#include <genlist/gendlist.h>
#include <genlist/genlistalloc.h>

typedef struct gadl_list_s {
	gdl_list_t dlist;
	gla_t allocator;

	size_t elem_size;

	void *alloc_ctx, *user_ctx;

	unsigned int dyn_alloced:1;
} gadl_list_t;

typedef struct {
	union {
		gdl_elem_t e;
		gla_t allocator;
	} link;
	char linked;
	ptrdiff_t size;
	char data[1];
} gadl_elem_t;


typedef struct gadl_iterator_s {
	gadl_elem_t *loop_elem, *prev, *next;
	size_t count;
} gadl_iterator_t;


typedef void (gadl_destruct_t)(void *item);

gadl_list_t *gadl_list_init(gadl_list_t *list, int elem_size, const gla_t *allocator, void *alloc_ctx);

void *gadl_new(gadl_list_t *list);
void *gadl_new0(gadl_list_t *list);

void gadl_free(void *item);
void gadl_destroy(void *item, gadl_destruct_t dfunc);

void *gadl_next_nth(void *item, int n);
void *gadl_prev_nth(void *item, int n);

void *gadl_nth(gadl_list_t *list, int n);
int gadl_index(void *item);

void gadl_append(gadl_list_t *list, void *item);
void gadl_insert(gadl_list_t *list, void *item);

void gadl_insert_before(void *at_item, void *item);
void gadl_insert_after(void *at_item, void *item);

void gadl_remove(void *item);

void gadl_reverse(gadl_list_t *list);



typedef int (*gadl_cmp_t)(void *listitem, void *data);
void *gadl_find(gadl_list_t *list, void *data, gadl_cmp_t cmpfunc);
void *gadl_find_next(void *item, void *data, gadl_cmp_t cmpfunc);

typedef int (*gadl_ctxcmp_t)(void *ctx, void *listitem, void *data);
void *gadl_ctxfind(gadl_list_t *list, void *data, gadl_ctxcmp_t cmpfunc, void *ctx);
void *gadl_ctxfind_next(void *item, void *data, gadl_ctxcmp_t cmpfunc, void *ctx);


extern gadl_elem_t gadl_addr_tmp_elem;
extern gadl_list_t gadl_addr_tmp_list;
/* (item_t*) -> (gadl_elem_t*)*/
#define gadl_gdl_elem_of(item) \
	((gadl_elem_t *)(((char *)(item)) \
	- ((char *)&(gadl_addr_tmp_elem.data) - (char *)&(gadl_addr_tmp_elem))))

/* (item_t*) -> (gadl_list_t*) */
#define gadl_gdl_list_of(elem) \
	((gadl_list_t *)(((char *)(elem->link.e.parent)) \
	- ((char *)&(gadl_addr_tmp_list.dlist) - (char *)&(gadl_addr_tmp_list))))

#define gadl_tmp_gdl_next_ptr(item_ptr)\
	gdl_next(((gadl_elem_t*)gadl_gdl_elem_of((item_ptr)))->link.e.parent, gadl_gdl_elem_of((item_ptr)))
#define gadl_tmp_gdl_prev_ptr(item_ptr)\
	gdl_prev(((gadl_elem_t*)gadl_gdl_elem_of((item_ptr)))->link.e.parent, gadl_gdl_elem_of((item_ptr)))

#define gadl_tmp_ptr_field_address_or_null(OBJECT_PTR, FIELD) \
	((OBJECT_PTR)? &((OBJECT_PTR)->FIELD) : NULL)

/* gadl_next(item_ptr); gadl_prev(item_ptr); - Get next/prev item from item's list,
   where item_ptr is the heap-allocated user data by address of char data[1] at gadl_elem_t.
   An example:
   gadl_list_t* list = gadl_list_init(NULL, sizeof(user_item_t), 0, 0);
   for(user_item_t* data = gadl_first(list); data; data = gadl_next(data)) { operate with (user_item_t*) data }
*/
#define gadl_next(item_ptr) \
	(gadl_gdl_elem_of(item_ptr)->linked? \
	gadl_tmp_ptr_field_address_or_null((gadl_elem_t*)(gadl_tmp_gdl_next_ptr((item_ptr))), data) : NULL )

#define gadl_prev(item_ptr) \
	(gadl_gdl_elem_of(item_ptr)->linked? \
	gadl_tmp_ptr_field_address_or_null((gadl_elem_t*)(gadl_tmp_gdl_prev_ptr((item_ptr))), data) : NULL)


#define gadl_foreach(list, iterator, loop_item) gadl_foreach_((list), (iterator), (loop_item))
#define gadl_foreach_(list, iterator, loop_item) \
	for( \
		iterator->loop_elem = (gadl_elem_t *)list->dlist.first, iterator->prev = NULL, iterator->next = gdl_next(&(list->dlist), iterator->loop_elem), iterator->count = 0, loop_item = (void *)&(iterator->loop_elem->data); \
		iterator->loop_elem != NULL; \
		iterator->loop_elem = iterator->next, iterator->next = gdl_next(&(list->dlist), iterator->next), iterator->count++, loop_item = (void *)&(iterator->loop_elem->data) \
	)

#define gadl_it_idx(iterator) ((iterator)->count)

int gadl_length(gadl_list_t *list);

void *gadl_first(gadl_list_t *list);
void *gadl_last(gadl_list_t *list);
gadl_list_t *gadl_parent(void *item);

void gadl_swap(void *itema, void *itemb);

#endif
