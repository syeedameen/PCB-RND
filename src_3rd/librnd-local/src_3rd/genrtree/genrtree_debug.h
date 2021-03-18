#include <stdio.h>

#ifndef RTR_DEBUG_NODE_COLOR
#	define RTR_DEBUG_NODE_COLOR fprintf(f, "dash 0xf0f0\ncolor #00ff00\n")
#endif

#ifndef RTR_ADJ
#	define RTR_ADJ 0.05
#endif

void RTR(dump_anim_)(FILE *f, RTR(t) *node, int *clr_node)
{
	int n;
	double adj = 0.0;
	RTR(t) *nd;

	for(nd = node->parent; nd != NULL; nd = nd->parent)
		adj += RTR_ADJ;

	if (!(*clr_node)) {
		RTR_DEBUG_NODE_COLOR;
		*clr_node = 1;
	}
	fprintf(f, "rect %f %f - %f %f\n", (double)node->bbox.x1+adj, (double)node->bbox.y1+adj, (double)node->bbox.x2-adj, (double)node->bbox.y2-adj);
	fprintf(f, "line %f %f %f %f\n", (double)node->bbox.x1+adj, (double)node->bbox.y1+adj, (double)node->bbox.x2-adj, (double)node->bbox.y2-adj);
	fprintf(f, "line %f %f %f %f\n", (double)node->bbox.x1+adj, (double)node->bbox.y2-adj, (double)node->bbox.x2-adj, (double)node->bbox.y1+adj);
	if (node->is_leaf) {
		*clr_node = 0;
		fprintf(f, "dash 0xffff\ncolor #ff0000\n");
		adj += RTR_ADJ;
		for(n = 0; n < node->used; n++) {
			fprintf(f, "rect %f %f - %f %f\n", (double)node->child.obj[n].box->x1+adj, (double)node->child.obj[n].box->y1+adj, (double)node->child.obj[n].box->x2-adj, (double)node->child.obj[n].box->y2-adj);
			fprintf(f, "line %f %f %f %f\n", (double)node->child.obj[n].box->x1+adj, (double)node->child.obj[n].box->y1+adj, (double)node->child.obj[n].box->x2-adj, (double)node->child.obj[n].box->y2-adj);
			fprintf(f, "line %f %f %f %f\n", (double)node->child.obj[n].box->x1+adj, (double)node->child.obj[n].box->y2-adj, (double)node->child.obj[n].box->x2-adj, (double)node->child.obj[n].box->y1+adj);
		}
	}
	else {
		for(n = 0; n < node->used; n++)
			RTR(dump_anim_)(f, node->child.node[n], clr_node);
	}
}

void RTR(dump_anim)(FILE *f, RTR(t) *node)
{
	int clr_node;
	fprintf(f, "scale 1 -1\n");
	fprintf(f, "viewport %f %f - %f %f\n", (double)node->bbox.x1 - 1.0, (double)node->bbox.y1 - 1.0, (double)node->bbox.x2 + 1.0, (double)node->bbox.y2 + 1.0);
	fprintf(f, "alpha 0.5\n");
	RTR_DEBUG_NODE_COLOR;
	clr_node = 1;
	RTR(dump_anim_)(f, node, &clr_node);
}

static void RTR(dump_ind)(FILE *f, int level, char *spaces, int maxspc)
{
	int n = level > maxspc ? maxspc : level;
	spaces[n] = '\0';
	fprintf(f, "%s", spaces);
	spaces[n] = ' ';
}

void RTR(dump_text_)(FILE *f, RTR(t) *node, int level, char *spaces, int maxspc, void (*print_obj)(FILE *f, void *obj))
{
	int n;

	/* print indentation */
	RTR(dump_ind)(f, level, spaces, maxspc);

	/* print node */
	fprintf(f, "%s used=%d size=%ld %g;%g %g;%g\n", (node->is_leaf ? "leaf" : "node"), node->used, (long)node->size, (double)node->bbox.x1, (double)node->bbox.y1, (double)node->bbox.x2, (double)node->bbox.y2);
	if (node->is_leaf) {
		for(n = 0; n < node->used; n++) {
			RTR(dump_ind)(f, level+1, spaces, maxspc);
			fprintf(f, "obj %g;%g %g;%g", (double)node->child.obj[n].box->x1, (double)node->child.obj[n].box->y1, (double)node->child.obj[n].box->x2, (double)node->child.obj[n].box->y2);
			if (print_obj != NULL)
				print_obj(f, node->child.obj[n].obj);
			fprintf(f, "\n");
		}
	}
	else {
		for(n = 0; n < node->used; n++)
			RTR(dump_text_)(f, node->child.node[n], level+1, spaces, maxspc, print_obj);
	}
}

void RTR(dump_text)(FILE *f, RTR(t) *node, void (*print_obj)(FILE *f, void *obj))
{
	char spaces[1024];
	memset(spaces, ' ', sizeof(spaces));
	RTR(dump_text_)(f, node, 0, spaces, sizeof(spaces)-1, print_obj);
}
