/*
    liblihata - list/hash/table format, symlink helpers
    Copyright (C) 2013  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Project URLs: http://repo.hu/projects/lihata
                  svn://repo.hu/lihata


   This file contains non-transparent symlink helpers.
*/
#include <stdlib.h>
#include "liblihata/dom.h"
#include "liblihata/tree.h"

int lht_tree_symlink_is_broken(lht_node_t *cwd, lht_node_t *sy)
{
	lht_doc_t *doc;
	lht_node_t *result;

	if (sy->type != LHT_SYMLINK)
		return -1;

	doc = cwd->doc != NULL ? cwd->doc : sy->doc;
	result = lht_tree_path_(doc, cwd, sy->data.text.value, 1, 0, NULL);

	if (result == NULL)
		return 1;

	cwd = result;
	return 0;
}

int lht_tree_has_symlink_(lht_node_t *n, int chk_broken)
{
	if (n->type == LHT_SYMLINK) {
		/* if we need to check for symlink only, return when the first is found */
		if (chk_broken == 0)
			return 1;

		/* else we need to test if it is broken - return on the first broken symlink */
		if (lht_tree_symlink_is_broken(n->parent, n))
			return 1;
	}
	return 0;
}

int lht_tree_has_symlink(lht_node_t *tree, int chk_broken)
{
	lht_dom_iterator_t it;
	lht_node_t *n;

	if (lht_tree_has_symlink_(tree, chk_broken))
		return 1;

	/* a depth-first-search for symlinks or broken symlinks */
	for(n = lht_dom_first(&it, tree); n != NULL; n = lht_dom_next(&it)) {

		if (lht_tree_has_symlink_(n, chk_broken))
			return 1;

		/* if we are still here, we need to descend */
		if ((n->type != LHT_TEXT) && (lht_tree_has_symlink(n, chk_broken) != 0))
				return 1;
	}

	/* finished the DFS without any hit */
	return 0;
}
