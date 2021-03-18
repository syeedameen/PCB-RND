#include "gendlist.h"

#ifndef TDL_LIST_U
#define TDL_LIST_U
#endif

typedef union TDL_LIST_U {
	gdl_list_t lst;
} TDL_LIST_T;

TDL_FUNC TDL_ITEM_T *TDL(next)(TDL_ITEM_T *item);
TDL_FUNC TDL_ITEM_T *TDL(prev)(TDL_ITEM_T *item);
TDL_FUNC TDL_ITEM_T *TDL(next_nth)(TDL_ITEM_T *item, TDL_SIZE_T n);
TDL_FUNC TDL_ITEM_T *TDL(prev_nth)(TDL_ITEM_T *item, TDL_SIZE_T n);
TDL_FUNC TDL_ITEM_T *TDL(nth)(TDL_LIST_T *list, TDL_SIZE_T n);
TDL_FUNC TDL_LIST_T *TDL(parent)(TDL_ITEM_T *item);

TDL_FUNC TDL_ITEM_T *TDL(first)(TDL_LIST_T *list);
TDL_FUNC TDL_ITEM_T *TDL(last)(TDL_LIST_T *list);

typedef int (*TDL(cmp_t))(TDL_ITEM_T *item, void *data);
typedef int (*TDL(ctxcmp_t))(void *ctx, TDL_ITEM_T *item, void *data);
TDL_FUNC TDL_ITEM_T *TDL(find)(TDL_LIST_T *list, void *data, TDL(cmp_t) compar);
TDL_FUNC TDL_ITEM_T *TDL(find_next)(TDL_ITEM_T *last, void *data, TDL(cmp_t) compar);
TDL_FUNC TDL_ITEM_T *TDL(ctxfind)(TDL_LIST_T *list, void *data, TDL(ctxcmp_t) compar, void *ctx);
TDL_FUNC TDL_ITEM_T *TDL(ctxfind_next)(TDL_ITEM_T *last, void *data, TDL(ctxcmp_t) compar, void *ctx);

TDL_FUNC int TDL(append)(TDL_LIST_T *list, TDL_ITEM_T *item);
TDL_FUNC int TDL(insert)(TDL_LIST_T *list, TDL_ITEM_T *item);
TDL_FUNC int TDL(insert_before)(TDL_LIST_T *list, TDL_ITEM_T *item_at, TDL_ITEM_T *item);
TDL_FUNC int TDL(insert_after)(TDL_LIST_T *list, TDL_ITEM_T *item_at, TDL_ITEM_T *item);
TDL_FUNC int TDL(remove)(TDL_ITEM_T *item);
TDL_FUNC int TDL(swap)(TDL_ITEM_T *itema, TDL_ITEM_T *itemb);
TDL_FUNC int TDL(check)(TDL_LIST_T *list);
TDL_FUNC TDL_SIZE_T TDL(length)(const TDL_LIST_T *list);
TDL_FUNC TDL_SIZE_T TDL(index)(TDL_ITEM_T *item);


TDL_FUNC void TDL(reverse)(TDL_LIST_T *list);


