#include <assert.h>

int RTR(delete)(RTR(t) *dst, void *obj, RTR(box_t) *box)
{
	void *target = obj;
	RTR(t) *parent, *up;
	int n, d;

	parent = RTR(search_parent)(dst, obj, box);
	if (parent == NULL)
		return -1;

	/* single-child nodes should be free'd and removed */
	ascend:;
	up = parent->parent; /* NULL means parent is the root - don't free that! */
	if (parent->used == 1) {
		if (up != NULL) {
			target = parent;
			free(parent);
			parent = up;
			goto ascend;
		}
		else {
			/* special case: root became empty - must be a leaf now */
			parent->is_leaf = 1;
			parent->size = 0;
			parent->used = 0;
			return 0;
		}
	}

	/* search and remove the target */
	if (parent->is_leaf) {
		for(d = -1, n = 0; n < parent->used; n++) {
			if (parent->child.obj[n].obj == target) {
				d = n;
				break;
			}
		}
		assert(d >= 0);
		if (d < 0)
			return -1;
		for(n = d; n < parent->used-1; n++) {
			parent->child.obj[n].obj = parent->child.obj[n+1].obj;
			parent->child.obj[n].box = parent->child.obj[n+1].box;
		}
	}
	else {
		for(d = -1, n = 0; n < parent->used; n++) {
			if (parent->child.node[n] == target) {
				d = n;
				break;
			}
		}
		assert(d >= 0);
		if (d < 0)
			return -1;
		for(n = d; n < parent->used-1; n++)
			parent->child.node[n] = parent->child.node[n+1];
	}
	parent->used--;

	/* ascend to decrease subtree size and recalculate the bounding box */
	for(; parent != NULL; parent = parent->parent) {
		parent->size--;
		if (parent->used <= 0)
			parent->bbox.x1 = parent->bbox.y1 = parent->bbox.x2 = parent->bbox.y2 = 0;
		else
			RTR(recalc_bbox)(parent);
	}

	return 0;
}
