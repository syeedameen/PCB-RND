#include <stdlib.h>

#ifndef RTR_MIN
#	define RTR_MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef RTR_MAX
#	define RTR_MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

RTR(privfunc) double RTR(dist2)(RTR(coord_t) x1, RTR(coord_t) y1, RTR(coord_t) x2, RTR(coord_t) y2)
{
	double dx = (x2-x1), dy = (y2-y1);
	return dx*dx + dy*dy;
}

void RTR(box_bump)(RTR(box_t) *dst, const RTR(box_t) *src)
{
	dst->x1 = RTR_MIN(dst->x1, src->x1);
	dst->y1 = RTR_MIN(dst->y1, src->y1);
	dst->x2 = RTR_MAX(dst->x2, src->x2);
	dst->y2 = RTR_MAX(dst->y2, src->y2);
}

RTR(privfunc) void RTR(recalc_bbox)(RTR(t) *node)
{
	int n;
	if (node->is_leaf) {
		node->bbox = *node->child.obj[0].box;
		for(n = 1; n < node->used; n++)
			RTR(box_bump)(&node->bbox, node->child.obj[n].box);
	}
	else {
		node->bbox = node->child.node[0]->bbox;
		for(n = 1; n < node->used; n++)
			RTR(box_bump)(&node->bbox, &node->child.node[n]->bbox);
	}
}

#if 0
	RTR(coord_t) ccx[2], ccy[2]; /* cluster centers */
	RTR(coord_t) pccx[2], pccy[2]; /* previous cluster centers */
	RTR(coord_t) ocx[RTR(size)], ocy[RTR(size)]; /* object centers */
#endif

/* Split a leaf node into two new leaf nodes, converting the original node
   into a non-leaf node. Implements the paper 'A new enhancement to the R-tree
   node splitting' by Amer F. Badarneh et al. */
RTR(privfunc) void RTR(split_leaf)(RTR(t) *node, RTR(t) **out_new1, RTR(t) **out_new2)
{
	int g, n, i, minfill=2;
	int grp[RTR(size)] = {0}; /* in which group, 0 or 1, each obj is in */
	RTR(t) *nn[2]; /* split execution: the two new groups */
	int seed[2], filled[2];
	RTR(coord_t) lowest, highest, low, high;
	double gdist[RTR(size)][2]; /* distance from group 1 and group 2 */

	/*** LPickSeeds ***/
	lowest = node->child.obj[0].box->x1 + node->child.obj[0].box->y1;
	highest = node->child.obj[0].box->x2 + node->child.obj[0].box->y2;
	seed[0] = seed[1] = 0;
	for(n = 1; n < node->used; n++) {
		low = node->child.obj[n].box->x1 + node->child.obj[n].box->y1;
		high = node->child.obj[n].box->x2 + node->child.obj[n].box->y2;
		if (low < lowest) {
			lowest = low;
			seed[0] = n;
		}
		if (high > highest) {
			highest = high;
			seed[1] = n;
		}
	}
	grp[seed[0]] = 0;
	grp[seed[1]] = 1;
	gdist[seed[0]][0] = gdist[seed[0]][1] = gdist[seed[1]][0] = gdist[seed[1]][1] = -1; /* do not try to put these in any other group later */

	/*** NDistribute (without sort because there are only a few items) ***/
	/* calculate distances from groups */
	for(n = 0; n < node->used; n++) {
		if ((n == seed[0]) || (n == seed[1])) continue;
		gdist[n][0] = RTR(dist2)(node->child.obj[seed[0]].box->x1, node->child.obj[seed[0]].box->y1, node->child.obj[n].box->x2, node->child.obj[n].box->y2);
		gdist[n][1] = RTR(dist2)(node->child.obj[seed[1]].box->x2, node->child.obj[seed[1]].box->y2, node->child.obj[n].box->x1, node->child.obj[n].box->y1);
	}

	/* assign the best minfill items */
	filled[0] = filled[1] = 0;
	for(i = 0; (filled[0] < minfill) && (filled[1] < minfill) && (i < node->used); i++) {
		double bestd[2];
		int bestn[2] = {-1, -1};

		/* pick the best from the remaining items for each group... */
		for(n = 0; n < node->used; n++) {
			for(g = 0; g < 2; g++) {
				if (gdist[n][g] >= 0) {
					if ((bestn[g] < 0) || (gdist[n][g] < bestd[g])) {
						bestd[g] = gdist[n][g];
						bestn[g] = n;
					}
				}
			}
		}

		/* ... move the best */
		for(g = 0; g < 2; g++) {
			n = bestn[g];
			if ((n < 0) || (gdist[n][0] < 0)) continue; /* no best for this group, or the best in g==1 is already assigned to the other group (happened in g==0) */
			if (filled[g] >= minfill) continue;
			grp[n] = g;
			gdist[n][0] = gdist[n][1] = -1; /* remove from further distancing */
			filled[g]++;
		}
	}

	/* put the remaining items in the better group */
	for(n = 0; n < node->used; n++) {
		if (gdist[n][0] < 0) continue; /* already assigned */
		grp[n] = (gdist[n][0] < gdist[n][1]) ? 0 : 1;
	}


	/*** the actual split: introduce two new nodes under 'node' ***/

	/* allocate the two new nodes */
	for(g = 0; g < 2; g++) {
		nn[g] = calloc(sizeof(RTR(t)), 1);
		nn[g]->parent = node;
		nn[g]->is_leaf = 1;
	}

	/* insert objects */
	for(n = 0; n < node->used; n++) {
		g = grp[n];
		nn[g]->child.obj[(int)nn[g]->used].box = node->child.obj[n].box;
		nn[g]->child.obj[(int)nn[g]->used].obj = node->child.obj[n].box;
		nn[g]->used++;
		nn[g]->size++;
	}

	for(g = 0; g < 2; g++)
		RTR(recalc_bbox)(nn[g]);

	/* update parent */
	node->is_leaf = 0;
	node->used = 2;
	node->child.node[0] = nn[0];
	node->child.node[1] = nn[1];

	*out_new1 = nn[0];
	*out_new2 = nn[1];
}

/* Return a penalty score based on how much box would increase the area of
   node's bbox; the smaller the value is the lower the cost is. */
RTR(privfunc) double RTR(score)(RTR(t) *node, RTR(box_t) *box)
{
	double newarea, oldarea;
	RTR(box_t) newbox;

	oldarea = (double)(node->bbox.x2 - node->bbox.x1) * (double)(node->bbox.y2 - node->bbox.y1);

	newbox.x1 = RTR_MIN(node->bbox.x1, box->x1);
	newbox.y1 = RTR_MIN(node->bbox.y1, box->y1);
	newbox.x2 = RTR_MAX(node->bbox.x2, box->x2);
	newbox.y2 = RTR_MAX(node->bbox.y2, box->y2);
	newarea = (double)(newbox.x2 - newbox.x1) * (double)(newbox.y2 - newbox.y1);

	return newarea - oldarea;
}

/* recursively increase or decrease the size of the subtree, ascending up to
   the root */
RTR(privfunc) void RTR(bump_size)(RTR(t) *node, int delta)
{
	for(;node != NULL;node = node->parent)
		node->size += delta;
}

/* recursively update the bbox up to the the root */
RTR(privfunc) void RTR(bump_bbox)(RTR(t) *node, const RTR(box_t) *with)
{
	for(;node != NULL;node = node->parent) {
		RTR(box_bump)(&node->bbox, with);
		with = &node->bbox;
	}
}

int RTR(box_contains)(const RTR(box_t) *big, const RTR(box_t) *small)
{
	if (small->x1 < big->x1) return 0;
	if (small->y1 < big->y1) return 0;
	if (small->x2 > big->x2) return 0;
	if (small->y2 > big->y2) return 0;
	return 1;
}


void RTR(insert_)(RTR(t) *dst, void *obj, RTR(box_t) *box, int force)
{
	descend:;
	if (dst->is_leaf) {
		/* Either src bbox is within dst bbox, or force is 1; add the src here */
		if (dst->used == RTR(size)) { /* if dst grew too large, split it and retry in the better half */
			RTR(t) *n1, *n2;
			RTR(split_leaf)(dst, &n1, &n2);
			if (RTR(score)(n1, box) < RTR(score)(n2, box))
				dst = n1;
			else
				dst = n2;
			force = 0;
			goto descend;
		}
		else { /* there's room in this leaf */
			dst->child.obj[(int)dst->used].box = box;
			dst->child.obj[(int)dst->used].obj = obj;
			dst->used++;
			RTR(bump_size)(dst, +1);
			if (dst->used == 1) {
				/* single-entry leaf: node bbox is child's bbox */
				dst->bbox = *box;
			}
			RTR(bump_bbox)(dst, box);
		}
	}
	else {
		int n, bestn;
		double best, curr;

		assert(dst->used > 0);

		/* insert in an object into a non-leaf node */
		if (force)
			RTR(box_bump)(&dst->bbox, box);
		/* else: box is fully within dst->bbox already */

		/* if box is encolsed within any of the existing children, descend there */
		for(n = 0; n < dst->used; n++) {
			if (RTR(box_contains)(&dst->child.node[n]->bbox, box)) {
				dst = dst->child.node[n];
				force = 0;
				goto descend;
			}
		}

		/* not within any existing box; add a new leaf, if possible */
		if (dst->child.node[0]->is_leaf && (dst->used < RTR(size))) {
			RTR(t) *newn = malloc(sizeof(RTR(t)));

			dst->child.node[(int)dst->used] = newn;
			dst->used++;

			newn->bbox = *box;
			newn->parent = dst;
			newn->is_leaf = 1;
			newn->size = 0;
			newn->used = 1;
			newn->child.obj[0].box = box;
			newn->child.obj[0].obj = obj;
			RTR(bump_size)(newn, +1);
			RTR(bump_bbox)(dst, box);
			return;
		}

		/* did not fit in any child, did not have room for a new child - have
		   to enlarge one of the existing children (pick the best one) */
		best = RTR(score)(dst->child.node[0], box);
		bestn = 0;
		for(n = 1; n < dst->used; n++) {
			curr = RTR(score)(dst->child.node[n], box);
			if (curr < best) {
				best = curr;
				bestn = n;
			}
		}
		dst = dst->child.node[bestn];
		force = 1;
		goto descend;
	}
}

void RTR(insert)(RTR(t) *dst, void *obj, RTR(box_t) *box)
{
	RTR(insert_)(dst, obj, box, 0);
}

#ifdef NDEBUG
#	define CHECK(cond) if (!(cond)) { return 1; }
#else
#	define CHECK(cond) assert(cond)
#endif

RTR(privfunc) int RTR(check_)(RTR(t) *node)
{
	int n;
	RTR(box_t) real;


	/* calculate the real bbox as it appears locally */
	if (node->is_leaf) {
		if ((node->parent == NULL) && (node->used == 0)) {
			/* special case: empty root: must be a leaf, but 0 children allowed */
			return 0;
		}
		CHECK((node->used > 0) && (node->used <= RTR(size)));
		real = *node->child.obj[0].box;
		for(n = 1; n < node->used; n++)
			RTR(box_bump)(&real, node->child.obj[n].box);
	}
	else {
		CHECK((node->used > 0) && (node->used <= RTR(size)));
		real = node->child.node[0]->bbox;
		for(n = 1; n < node->used; n++)
			RTR(box_bump)(&real, &node->child.node[n]->bbox);
	}

	/* local bbpx check */
	CHECK(real.x1 == node->bbox.x1);
	CHECK(real.x2 == node->bbox.x2);
	CHECK(real.y1 == node->bbox.y1);
	CHECK(real.y2 == node->bbox.y2);

	/* if not a leaf, recurse to check the subtree */
	if (!node->is_leaf) {
		for(n = 0; n < node->used; n++)
			if (RTR(check_)(node->child.node[n]) != 0)
				return 1;
	}

	/* all went fine, no error found */
	return 0;
}

int RTR(check)(RTR(t) *node)
{
	return RTR(check_)(node);
}


#undef CHECK

void RTR(init)(RTR(t) *root)
{
	memset(root, 0, sizeof(RTR(t)));
	root->is_leaf = 1;
}

RTR(privfunc) void RTR(free)(RTR(t) *node, int do_free_node, void *ctx, void (*leaf_free)(void *ctx, void *obj))
{
	int n;
	if (node->is_leaf) {
		if (leaf_free != NULL)
			for(n = 0; n < node->used; n++)
				leaf_free(ctx, node->child.obj[n].obj);
	}
	else {
		for(n = 0; n < node->used; n++)
			RTR(free)(node->child.node[n], 1, ctx, leaf_free);
	}
	if (do_free_node)
		free(node);
}

void RTR(uninit_free_leaves)(RTR(t) *root, void *ctx, void (*leaf_free)(void *ctx, void *obj))
{
	RTR(free)(root, 0, ctx, leaf_free);
}

void RTR(uninit)(RTR(t) *root)
{
	RTR(free)(root, 0, NULL, NULL);
}
