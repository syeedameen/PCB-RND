#include <assert.h>

int RTR(box_touch)(const RTR(box_t) *db, const RTR(box_t) *query)
{
	if (db->x1 > query->x2) return 0;
	if (db->x2 < query->x1) return 0;
	if (db->y1 > query->y2) return 0;
	if (db->y2 < query->y1) return 0;
	return 1;
}

#define rtr_proc_res(res, inc_cnt) \
	do { \
		if ((inc_cnt) && (res & RTRU(DIR_FOUND))) \
			cnt++; \
		state |= res; \
		if (res & RTRU(DIR_STOP)) { \
			if (last_node != NULL) \
				*last_node = node; \
			goto ret; \
		} \
	} while(0)

#define rtr_descend(dnode) \
	do { \
		RTR(cardinal_t) dcnt; \
		res = RTR(search_any_nd)(dnode, query, found_node, found_obj, ctx, &dcnt, last_node); \
		cnt += dcnt; \
		state |= res; \
		if (res & RTRU(DIR_STOP)) \
			goto ret; \
	} while(0)

RTR(privfunc) RTR(dir_t) RTR(search_any_nd)(RTR(t) *node, const RTR(box_t) *query, RTR(cb_t) *found_node, RTR(cb_t) *found_obj, void *ctx, RTR(cardinal_t) *out_cnt, RTR(t) **last_node)
{
	RTR(cardinal_t) cnt = 0;
	RTR(dir_t) res, state = 0;
	int n;

	if ((node == NULL) || (node->size < 1))
		goto ret;

	if (query != NULL) {
		/* search for a box */
		if (node->is_leaf) {
			if (found_obj != NULL) {
				for(n = 0; n < node->used; n++) {
					if (RTR(box_touch)(node->child.obj[n].box, query)) {
						res = found_obj(ctx, node->child.obj[n].obj, node->child.obj[n].box);
						rtr_proc_res(res, 1);
					}
				}
			}
			else { /* no callback */
				for(n = 0; n < node->used; n++) {
					if (RTR(box_touch)(node->child.obj[n].box, query)) {
						res |= RTRU(DIR_FOUND);
						cnt++;
					}
				}
			}
		}
		else {
			/* tree node */
			if (found_node != NULL) {
				for(n = 0; n < node->used; n++) {
					if (RTR(box_touch)(&node->child.node[n]->bbox, query)) {
						res = found_node(ctx, NULL, &node->child.node[n]->bbox);
						rtr_proc_res(res, 0);
						rtr_descend(node->child.node[n]);
					}
				}
			}
			else { /* no callback */
				for(n = 0; n < node->used; n++)
					if (RTR(box_touch)(&node->child.node[n]->bbox, query))
						rtr_descend(node->child.node[n]);
			}
		}
	}
	else {
		/* iterate over everything */
		if (node->is_leaf) {
			if (found_obj != NULL) {
				for(n = 0; n < node->used; n++) {
					res = found_obj(ctx, node->child.obj[n].obj, node->child.obj[n].box);
					rtr_proc_res(res, 1);
				}
			}
			else { /* no callback */
				for(n = 0; n < node->used; n++) {
					res |= RTRU(DIR_FOUND);
					cnt++;
				}
			}
		}
		else {
			/* tree node */
			if (found_node != NULL) {
				for(n = 0; n < node->used; n++) {
					res = found_node(ctx, NULL, &node->child.node[n]->bbox);
					rtr_proc_res(res, 0);
					rtr_descend(node->child.node[n]);
				}
			}
			else { /* no callback */
				for(n = 0; n < node->used; n++)
					rtr_descend(node->child.node[n]);
			}
		}
	}

	ret:;
	if (out_cnt != NULL)
		*out_cnt = cnt;
	return state;
}

#undef rtr_proc_res
#undef rtr_descend

RTR(dir_t) RTR(search_any)(RTR(t) *node, const RTR(box_t) *query, RTR(cb_t) *found_node, RTR(cb_t) *found_obj, void *ctx, RTR(cardinal_t) *out_cnt)
{
	return RTR(search_any_nd)(node, query, found_node, found_obj, ctx, out_cnt, NULL);
}

RTR(dir_t) RTR(search_obj)(RTR(t) *node, const RTR(box_t) *query, RTR(cb_t) *found_obj, void *ctx)
{
	return RTR(search_any_nd)(node, query, NULL, found_obj, ctx, NULL, NULL);
}


RTR(privfunc) RTR(dir_t) RTR(search_parent_ocb)(void *target, void *obj, const RTR(box_t) *box)
{
	if (obj == target)
		return RTRU(DIR_FOUND) | RTRU(DIR_STOP);
	return 0;
}

RTR(t) *RTR(search_parent)(RTR(t) *node, void *obj, RTR(box_t) *obj_box)
{
	RTR(t) *parent;
	RTR(dir_t) res = RTR(search_any_nd)(node, obj_box, NULL, RTR(search_parent_ocb), obj, NULL, &parent);
	if (res & RTRU(DIR_FOUND))
		return parent;
	return NULL;
}

RTR(privfunc) RTR(dir_t) RTR(is_box_empty_ocb)(void *target, void *obj, const RTR(box_t) *box)
{
	return RTRU(DIR_FOUND) | RTRU(DIR_STOP);
}

int RTR(is_box_empty)(RTR(t) *node, const RTR(box_t) *box)
{
	RTR(dir_t) res = RTR(search_any_nd)(node, box, NULL, RTR(is_box_empty_ocb), NULL, NULL, NULL);
	return !(res & RTRU(DIR_FOUND));
}

#define N    (it->stack[it->used-1].n)
#define NODE (it->stack[it->used-1].node)

void *RTR(next)(RTR(it_t) *it)
{
	retry:;

	if (it->used == 0)
		return NULL;

	if (NODE->is_leaf) {
		while(N < NODE->used) {
			if (RTR(box_touch)(NODE->child.obj[N].box, &it->query)) {
				N++;
				it->cnt++;
				return NODE->child.obj[N-1].obj;
			}
			N++;
		}
		/* ran out of leaf objects, pop */
		it->used--;
		goto retry;
	}
	else {
		/* tree node */
		while(N < NODE->used) {
			if (RTR(box_touch)(&NODE->child.node[N]->bbox, &it->query)) {
				/* found a new node we shall descend into - push */
				assert(it->used < RTR(stack_max)); /* stack overflow */
				it->stack[it->used].node = NODE->child.node[N];
				it->stack[it->used].n = 0;
				N++;
				it->used++;
				goto retry;
			}
			N++;
		}
		/* ran out of non-leaf objects, pop */
		it->used--;
		goto retry;
	}
}


void *RTR(first)(RTR(it_t) *it, const RTR(t) *root, const RTR(box_t) *query)
{
	it->cnt = 0;
	it->stack[0].node = root;
	it->stack[0].n = 0;
	it->used = 1;
	it->query = *query;
	return RTR(next)(it);
}


void *RTR(all_next)(RTR(it_t) *it)
{
	retry:;

	if (it->used == 0)
		return NULL;

	if (NODE->is_leaf) {
		while(N < NODE->used) {
			N++;
			it->cnt++;
			return NODE->child.obj[N-1].obj;
		}
		/* ran out of leaf objects, pop */
		it->used--;
		goto retry;
	}
	else {
		/* tree node */
		while(N < NODE->used) {
			/* found a new node we shall descend into - push */
			assert(it->used < RTR(stack_max)); /* stack overflow */
			it->stack[it->used].node = NODE->child.node[N];
			it->stack[it->used].n = 0;
			N++;
			it->used++;
			goto retry;
		}
		/* ran out of non-leaf objects, pop */
		it->used--;
		goto retry;
	}
}

void *RTR(all_first)(RTR(it_t) *it, const RTR(t) *root)
{
	it->cnt = 0;
	it->stack[0].node = root;
	it->stack[0].n = 0;
	it->used = 1;
	return RTR(all_next)(it);
}

#undef N
#undef NODE
