#ifndef GENSEXPR_LOC_H
#define GENSEXPR_LOC_H

#define GSX(x) gsxl_ ## x
#define GENSEXPR_WANT_LOC

#include <gensexpr/gensexpr_impl.h>

/* Initialize a gsx*_dom_t *dom with an user struct type; the first field of
   the user struct must be gsx*_node_t */
#define gsxl_init(dom, struct_type) gsxl_init_(dom, sizeof(struct_type))

/* Allocate and initialize a dom with an user struct type; the first field of
   the user struct must be gsx_node_t. Evaluates to a (gsx_dom_t *); uses
   malloc() for the allocation */
#define gsxl_alloc(dom, struct_type) gsxl_alloc_(sizeof(struct_type))

#define gsxl_children(node)   ((void *)(((gsxl_node_t *)(node))->children))
#define gsxl_parent(node)     ((void *)(((gsxl_node_t *)(node))->parent))
#define gsxl_next(node)       ((void *)(((gsxl_node_t *)(node))->next))


#undef GSX
#undef GENSEXPR_WANT_LOC

#endif
