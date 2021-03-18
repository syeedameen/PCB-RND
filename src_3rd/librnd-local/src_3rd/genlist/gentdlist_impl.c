#include "gendlist.h"

/******** pointer return functions *******/
#undef assert
#define assert(x) \
	do { \
		if (!(x)) \
			return NULL; \
	} while(0)


TDL_FUNC TDL_ITEM_T *TDL(next)(TDL_ITEM_T *item)    { return gdl_next(item->TDL_FIELD.parent, item); }
TDL_FUNC TDL_ITEM_T *TDL(prev)(TDL_ITEM_T *item)    { return gdl_prev(item->TDL_FIELD.parent, item); }
TDL_FUNC TDL_LIST_T *TDL(parent)(TDL_ITEM_T *item)  { return (TDL_LIST_T *)(item->TDL_FIELD.parent); }
TDL_FUNC TDL_ITEM_T *TDL(first)(TDL_LIST_T *list)   { return gdl_first(&list->lst); }
TDL_FUNC TDL_ITEM_T *TDL(last)(TDL_LIST_T *list)    { return gdl_last(&list->lst); }

TDL_FUNC TDL_ITEM_T *TDL(next_nth)(TDL_ITEM_T *item, TDL_SIZE_T n)
{
	TDL_ITEM_T *res;
	gdl_next_nth(item->TDL_FIELD.parent, item, n, &res);
	return res;
}

TDL_FUNC TDL_ITEM_T *TDL(prev_nth)(TDL_ITEM_T *item, TDL_SIZE_T n)
{
	TDL_ITEM_T *res;
	gdl_prev_nth(item->TDL_FIELD.parent, item, n, &res);
	return res;
}

TDL_FUNC TDL_ITEM_T *TDL(nth)(TDL_LIST_T *list, TDL_SIZE_T n)
{
	TDL_ITEM_T *res;
	gdl_nth(&list->lst, n, &res);
	return res;
}

TDL_FUNC TDL_ITEM_T *TDL(find)(TDL_LIST_T *list, void *data, TDL(cmp_t) compar)
{
	TDL_ITEM_T *res;
	gdl_find(&list->lst, data, compar, &res);
	return res;
}

TDL_FUNC TDL_ITEM_T *TDL(find_next)(TDL_ITEM_T *last, void *data, TDL(cmp_t) compar)
{
	TDL_ITEM_T *res;
	gdl_find_next(last->TDL_FIELD.parent, last, data, compar, &res);
	return res;
}

TDL_FUNC TDL_ITEM_T *TDL(ctxfind)(TDL_LIST_T *list, void *data, TDL(ctxcmp_t) compar, void *ctx)
{
	TDL_ITEM_T *res;
	gdl_ctxfind(&list->lst, data, compar, ctx, &res);
	return res;
}

TDL_FUNC TDL_ITEM_T *TDL(ctxfind_next)(TDL_ITEM_T *last, void *data, TDL(ctxcmp_t) compar, void *ctx)
{
	TDL_ITEM_T *res;
	gdl_ctxfind_next(last->TDL_FIELD.parent, last, data, compar, ctx, &res);
	return res;
}

/******** integer return functions *******/
#undef assert
#define assert(x) \
	do { \
		if (!(x)) \
			return -1; \
	} while(0)

TDL_FUNC int TDL(append)(TDL_LIST_T *list, TDL_ITEM_T *item)
{
	gdl_append(&list->lst, item, TDL_FIELD);
	return 0;
}

TDL_FUNC int TDL(insert)(TDL_LIST_T *list, TDL_ITEM_T *item)
{
	gdl_insert(&list->lst, item, TDL_FIELD);
	return 0;
}

TDL_FUNC int TDL(insert_before)(TDL_LIST_T *list, TDL_ITEM_T *item_at, TDL_ITEM_T *item)
{
	gdl_insert_before(&list->lst, item_at, item, TDL_FIELD);
	return 0;
}

TDL_FUNC int TDL(insert_after)(TDL_LIST_T *list, TDL_ITEM_T *item_at, TDL_ITEM_T *item)
{
	gdl_insert_after(&list->lst, item_at, item, TDL_FIELD);
	return 0;
}

TDL_FUNC int TDL(remove)(TDL_ITEM_T *item)
{
	if (item->TDL_FIELD.parent == 0)
		return -1;
	gdl_remove(item->TDL_FIELD.parent, item, TDL_FIELD);
	return 0;
}

TDL_FUNC int TDL(swap)(TDL_ITEM_T *itema, TDL_ITEM_T *itemb)
{
	gdl_swap(itema->TDL_FIELD.parent, itema, itemb, TDL_FIELD);
	return 0;
}

TDL_FUNC int TDL(check)(TDL_LIST_T *list)
{
	gdl_check(&list->lst);
	return 0;
}

TDL_FUNC TDL_SIZE_T TDL(length)(const TDL_LIST_T *list)
{
	return gdl_length(&list->lst);
}

TDL_FUNC TDL_SIZE_T TDL(index)(TDL_ITEM_T *item)
{
	TDL_SIZE_T res;
	gdl_index(item->TDL_FIELD.parent, item, TDL_FIELD, &res);
	return res;
}

/******** void functions *******/
#undef assert
TDL_FUNC void TDL(reverse)(TDL_LIST_T *list)  { gdl_reverse(&list->lst); }



