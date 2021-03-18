#ifndef GENSEXPR_NOLOC_H
#define GENSEXPR_NOLOC_H

#define GSX(x) gsxnl_ ## x
#undef GENSEXPR_WANT_LOC

#include <gensexpr/gensexpr_impl.h>

/* Initialize a gsx*_dom_t *dom with an user struct type; the first field of
   the user struct must be gsx*_node_t */
#define gsxnl_init(dom, struct_type) gsxnl_init_(dom, sizeof(struct_type))

/* Allocate and initialize a dom with an user struct type; the first field of
   the user struct must be gsx_node_t. Evaluates to a (gsx_dom_t *); uses
   malloc() for the allocation */
#define gsxnl_alloc(dom, struct_type) gsxnl_alloc_(sizeof(struct_type))

#define gsxnl_children(node)   ((void *)(((gsxnl_node_t *)(node))->children))
#define gsxnl_parent(node)     ((void *)(((gsxnl_node_t *)(node))->parent))
#define gsxnl_next(node)       ((void *)(((gsxnl_node_t *)(node))->next))

#undef GSX

#endif
