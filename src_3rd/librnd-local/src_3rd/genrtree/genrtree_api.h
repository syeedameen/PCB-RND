#ifndef RTREE_NO_TREE_TYPEDEFS
typedef struct RTR(s) RTR(t);
typedef struct RTR(it_s) RTR(it_t);
#endif

/* object bounding box */
typedef struct RTR(box_s) {
	RTR(coord_t) x1, y1, x2, y2;
} RTR(box_t);

/* internal object representation - the user of this API won't need this */
typedef struct RTR(obj_s) {
	RTR(box_t) *box;
	void *obj;
} RTR(obj_t);

/* Tree node; a whole tree is represented by the (user allocated) root node */
struct RTR(s) {
	RTR(box_t) bbox;
	RTR(t) *parent;
	RTR(cardinal_t) size; /* recursive number-of-children */
	char is_leaf;
	char used; /* number of children ptr used */
	union {
		RTR(t) *node[RTR(size)];    /* when non-leaf */
		RTR(obj_t) obj[RTR(size)];  /* when leaf */
	} child;
};

/* directions on return - bitmask */
typedef enum RTR(dir_e) {
	RTRU(DIR_FOUND) = 1,
	RTRU(DIR_STOP) = 2
} RTR(dir_t);

/* combined directions - all possible combinations (useful for switch() warnings) */
typedef enum RTR(dirc_e) {
	RTRU(DIR_NOT_FOUND_CONT) = 0,                              /* object not found or not accepted; continue the search */
	RTRU(DIR_FOUND_CONT)     = RTRU(DIR_FOUND),                /* object found or accepted; continue the search */
	RTRU(DIR_NOT_FOUND_STOP) = RTRU(DIR_STOP),                 /* object not found or not accepted; stop the search */
	RTRU(DIR_FOUND_STOP)     = RTRU(DIR_FOUND)|RTRU(DIR_STOP)  /* object found or accepted; stop the search */
} RTR(dirc_t);

/*** generic tree API ***/

/* Init user-allocated tree to empty */
void RTR(init)(RTR(t) *root);

/* Uninit the tree, leave root empty; free all memory allocated by the lib;
   free_leaves also calls user provided lead_free() to free data */
void RTR(uninit)(RTR(t) *root);
void RTR(uninit_free_leaves)(RTR(t) *root, void *ctx, void (*leaf_free)(void *ctx, void *obj));

/* Insert obj/box into the dst tree; box normally points into obj */
void RTR(insert)(RTR(t) *dst, void *obj, RTR(box_t) *box);

/* Run integrity checks on a node (or tree) recursively */
int RTR(check)(RTR(t) *node);

/*** low level box API ***/

/* Enlarge dst (if needed) to include src */
void RTR(box_bump)(RTR(box_t) *dst, const RTR(box_t) *src);

/* Returns 1 if big contains small, 0 if not */
int RTR(box_contains)(const RTR(box_t) *big, const RTR(box_t) *small);

/*** optional: delete API from genrtree_delete.h ***/

/* search callback function - implementation provided by the caller */
typedef RTR(dir_t) (RTR(cb_t))(void *ctx, void *obj, const RTR(box_t) *box);

/* Remove obj/box from the tree named in dst; return 0 on success, -1 on error */
int RTR(delete)(RTR(t) *dst, void *obj, RTR(box_t) *box);

/*** optional: callback based search API from genrtree_search.h ***/

/* Search all objects (and optionally nodes) that have any overlap with
   the query box; return a bitmap of callback return values. If callback
   return includes DIR_STOP, abort the search immediately. */
RTR(dir_t) RTR(search_obj)(RTR(t) *node, const RTR(box_t) *query, RTR(cb_t) *found_obj, void *ctx);
RTR(dir_t) RTR(search_any)(RTR(t) *node, const RTR(box_t) *query, RTR(cb_t) *found_node, RTR(cb_t) *found_obj, void *ctx, RTR(cardinal_t) *out_cnt);

/* Search the object in the node (sub)tree and return the parent node it was
   found in (or NULL) */
RTR(t) *RTR(search_parent)(RTR(t) *node, void *obj, RTR(box_t) *obj_box);

/* Returns 1 if box does not overlap with any known object (0 otherhwise) */
int RTR(is_box_empty)(RTR(t) *node, const RTR(box_t) *box);

/*** optional: iterator/loop based search API from genrtree_search.h ***/

/* iterator that can be used in an iterative search */
struct RTR(it_s) {
	RTR(cardinal_t) cnt; /* number of objects found so far */
	RTR(box_t) query;

	/* stack */
	struct {
		const RTR(t) *node;
		int n; /* next idx to check on this level */
	} stack[RTR(stack_max)];
	int used;
};

/* Return the first object matching the query or NULL, if there is no match */
void *RTR(first)(RTR(it_t) *it, const RTR(t) *root, const RTR(box_t) *query);

/* Return the next object matching the query or NULL, if there is no more */
void *RTR(next)(RTR(it_t) *it);

/* Return the first object from the tree or NULL, if the tree is empty */
void *RTR(all_first)(RTR(it_t) *it, const RTR(t) *root);

/* Return the next object matching from the tree, if there is no more */
void *RTR(all_next)(RTR(it_t) *it);

/*** optional: box helpers from genrtree_search.h ***/

/* return 1 if db and query have at least one common point */
int RTR(box_touch)(const RTR(box_t) *db, const RTR(box_t) *query);

