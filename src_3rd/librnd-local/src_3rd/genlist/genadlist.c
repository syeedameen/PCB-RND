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

#include <string.h>
#include <assert.h>
#include <genlist/genadlist.h>

gadl_list_t *gadl_list_init(gadl_list_t *list, int elem_size, const gla_t *allocator, void *alloc_ctx)
{
	if (allocator == NULL)
		allocator = &gla_malloc;

	if (list == NULL) {
		list = allocator->alloc(alloc_ctx, sizeof(gadl_list_t));
		list->dyn_alloced = 1;
	}
	else
		list->dyn_alloced = 0;

	list->allocator = *allocator;
	list->alloc_ctx = alloc_ctx;
	list->elem_size = elem_size;

	memset(&(list->dlist), 0, sizeof(list->dlist));
	return list;
}

/* an elem may have two relation to lists:
 - (linked) member if a list:
     e.parent=list->dlist and e.next or e.prev set
 - weak conn to a list: allocated for a list or was last member of a list, but
   currently is not member of any list:
     e.parent=list->dlist and e.next=e.prev set=NULL
 Elements remember their last list connection. Such a weak conn can not
 be broken. Weak conn should not be dereferenced, tho: the list may have
 been free()'d meanwhile.
*/

#define elem_on_list(elem) (elem->linked)

#define detach(list, elem) \
do { \
	elem->linked = 0; \
	elem->link.allocator = list->allocator; \
} while(0)

#define preattach(elem) \
do { \
	elem->linked = 1; \
	elem->link.e.parent = elem->link.e.prev = elem->link.e.next = NULL; \
} while(0)

static gadl_elem_t *gadl_new_elem(gadl_list_t *list)
{
	gadl_elem_t *elem;
	elem = list->allocator.alloc(list->alloc_ctx, sizeof(gadl_elem_t) + list->elem_size - 1);
	if (elem == NULL)
		return NULL;
	elem->link.e.prev = elem->link.e.next = NULL;
	detach(list, elem);
	elem->size = list->elem_size;
	return elem;
}

void *gadl_new(gadl_list_t *list)
{
	gadl_elem_t *elem = gadl_new_elem(list);
	return &elem->data;
}

void *gadl_new0(gadl_list_t *list)
{
	gadl_elem_t *elem = gadl_new_elem(list);
	if (elem != NULL)
		memset(elem->data, 0, list->elem_size);
	return &elem->data;
}

gadl_elem_t gadl_addr_tmp_elem;
gadl_list_t gadl_addr_tmp_list;

#define elem_of(item) gadl_gdl_elem_of((item))
#define list_of(item) gadl_gdl_list_of((item))

void *gadl_next_nth(void *item, int n)
{
	gadl_elem_t *elem = elem_of(item);
	gadl_elem_t *res;

	gdl_next_nth(elem->link.e.parent, elem, n, &res);
	if (res == NULL)
		return NULL;
	return &res->data;
}

void *gadl_prev_nth(void *item, int n)
{
	gadl_elem_t *elem = elem_of(item);
	gadl_elem_t *res;

	gdl_prev_nth(elem->link.e.parent, elem, n, &res);
	if (res == NULL)
		return NULL;
	return &res->data;
}

void *gadl_nth(gadl_list_t *list, int n)
{
	gadl_elem_t *res;
	gdl_nth(&(list->dlist), n, &res);
	if (res == NULL)
		return NULL;
	return &(res->data);
}

int gadl_index(void *item)
{
	gadl_elem_t *elem = elem_of(item);
	gadl_list_t *list = list_of(elem);
	int res;

	if (!elem_on_list(elem))
		return -1;

	gdl_index(&(list->dlist), elem, link.e, &res);
	return res;
}

static void gadl_remove_elem(gadl_elem_t *elem, int destroy, gadl_destruct_t dfunc)
{
	gadl_list_t *orig_list = list_of(elem);

	if ((destroy) && (dfunc != NULL))
		dfunc(&elem->data);

	if (elem_on_list(elem)) {
		gdl_remove(&(orig_list->dlist), elem, link.e);
	}

	if (destroy) {
		if (elem_on_list(elem))
			orig_list->allocator.free(orig_list->alloc_ctx, elem);
		else
			elem->link.allocator.free(orig_list->alloc_ctx, elem);
	}
	else
		detach(orig_list, elem);
}

void gadl_free(void *item)
{
	gadl_elem_t *elem = elem_of(item);

	gadl_remove_elem(elem, 1, NULL);
}

void gadl_destroy(void *item, gadl_destruct_t dfunc)
{
	gadl_elem_t *elem = elem_of(item);

	gadl_remove_elem(elem, 1, dfunc);
}

void gadl_remove(void *item)
{
	gadl_elem_t *elem = elem_of(item);

	gadl_remove_elem(elem, 0, NULL);
}

void gadl_append(gadl_list_t *list, void *item)
{
	gadl_elem_t *elem = elem_of(item);

	gadl_remove_elem(elem, 0, NULL);

	preattach(elem);
	gdl_append(&(list->dlist), elem, link.e);
}

void gadl_insert(gadl_list_t *list, void *item)
{
	gadl_elem_t *elem = elem_of(item);

	gadl_remove_elem(elem, 0, NULL);

	preattach(elem);
	gdl_insert(&(list->dlist), elem, link.e);
}


void gadl_insert_before(void *at_item, void *item)
{
	gadl_elem_t *at_elem = elem_of(at_item);
	gadl_elem_t *elem    = elem_of(item);
	gadl_list_t *list    = list_of(at_elem);

	assert(elem_on_list(at_elem));

	gadl_remove_elem(elem, 0, NULL);

	preattach(elem);
	gdl_insert_before(&(list->dlist), at_elem, elem, link.e);
}

void gadl_insert_after(void *at_item, void *item)
{
	gadl_elem_t *at_elem = elem_of(at_item);
	gadl_elem_t *elem    = elem_of(item);
	gadl_list_t *list    = list_of(at_elem);

	assert(elem_on_list(at_elem));

	gadl_remove_elem(elem, 0, NULL);

	preattach(elem);
	gdl_insert_after(&(list->dlist), at_elem, elem, link.e);
}

void gadl_swap(void *itema, void *itemb)
{
	gadl_elem_t *elema = elem_of(itema);
	gadl_elem_t *elemb = elem_of(itemb);
	gadl_list_t *lista = list_of(elema);
	gadl_list_t *listb = list_of(elemb);

	assert(elem_on_list(elema));
	assert(elem_on_list(elemb));
	assert(lista == listb);

	gdl_swap(&(lista->dlist), elema, elemb, link.e);
}

void gadl_reverse(gadl_list_t *list)
{
	gdl_reverse(&(list->dlist));
}

void *gadl_find(gadl_list_t *list, void *data, gadl_cmp_t cmpfunc)
{
	gadl_elem_t *i;
	for(i = list->dlist.first; i != NULL; i = gdl_next(&(list->dlist), i))
		if (cmpfunc(i->data, data) == 0)
			return &(i->data);
	return NULL;
}

void *gadl_find_next(void *item, void *data, gadl_cmp_t cmpfunc)
{
	gadl_elem_t *i = elem_of(item);
	gadl_list_t *list = list_of(i);

	for(i = gdl_next(&(list->dlist), i); i != NULL; i = gdl_next(&(list->dlist), i))
		if (cmpfunc(i->data, data) == 0)
			return &(i->data);
	return NULL;
}

void *gadl_ctxfind(gadl_list_t *list, void *data, gadl_ctxcmp_t cmpfunc, void *ctx)
{
	gadl_elem_t *i;
	for(i = list->dlist.first; i != NULL; i = gdl_next(&(list->dlist), i))
		if (cmpfunc(ctx, i->data, data) == 0)
			return &(i->data);
	return NULL;
}

void *gadl_ctxfind_next(void *item, void *data, gadl_ctxcmp_t cmpfunc, void *ctx)
{
	gadl_elem_t *i = elem_of(item);
	gadl_list_t *list = list_of(i);

	for(i = gdl_next(&(list->dlist), i); i != NULL; i = gdl_next(&(list->dlist), i))
		if (cmpfunc(ctx, i->data, data) == 0)
			return &(i->data);
	return NULL;
}


int gadl_length(gadl_list_t *list)
{
	return list->dlist.length;
}

void *gadl_first(gadl_list_t *list)
{
	gadl_elem_t *i = list->dlist.first;

	if (i == NULL)
		return NULL;

	return &(i->data);
}

void *gadl_last(gadl_list_t *list)
{
	gadl_elem_t *i = list->dlist.last;

	if (i == NULL)
		return NULL;

	return &(i->data);
}

gadl_list_t *gadl_parent(void *item)
{
	gadl_elem_t *elem = elem_of(item);
	if (!elem->linked)
		return NULL;
	return list_of(elem);
}
