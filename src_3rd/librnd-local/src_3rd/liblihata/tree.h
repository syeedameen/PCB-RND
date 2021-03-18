#ifndef LIHATA_TREE_H
#define LIHATA_TREE_H
#include "liblihata/dom.h"

/* --- generic --- */

/* return non-zero if anc is an ancestor of node */
int lht_tree_is_under(lht_node_t *node, lht_node_t *anc);

/* Unlink the node from under its parent; the node is placed on the unlinked list of the document
   Unlinking means the node is floating within the document; it's not linked
   in the tree but is not forgotten. Unlinked nodes of a document is freed
   when the document is freed.
*/
lht_err_t lht_tree_unlink(lht_node_t *node);

/* Detach the node from under its parent; the node is left floating:
   not being part of any document. Warning: detaching large subtrees
   can be slow due to detaching of location info for the whole subtree. */
lht_err_t lht_tree_detach(lht_node_t *node);

/* Detach the node from under its parent; the node is left floating:
   not being part of any document. Put (already detached) newn
   in the same place in parent - this means:
    - same place in list (order will not change)
    - same row:col in table
    - in hash node will be detached and newn will be added (there's no order in hash)
   */
lht_err_t lht_tree_replace(lht_node_t *node, lht_node_t *newn);


/* Detach the node from under its parent and free it recursively */
lht_err_t lht_tree_del(lht_node_t *node);

/* resolve a path and return the matching node. cwd is used if path is relative.
   Symlinks:
     - followed along the path until the last portion of the path
     - if follow_sy is zero, the resulting node is returned as-is even if it is a symlink
     - if follow_sy is non-zero and the resulting node would be a symlink, search is restarted using the value of the symlink
   The _ suffixed call expects cwd to be a node and exposes depth (should be 0 if the call is not the follow-up for a symlink lookup)
   Error code is stored in *err if err is not NULL.
*/
lht_node_t *lht_tree_path(lht_doc_t *doc, const char *cwd, const char *path, int follow_sy, lht_err_t *err);
lht_node_t *lht_tree_path_(lht_doc_t *doc, const lht_node_t *cwd, const char *path_, int follow_sy, int depth, lht_err_t *err);

/* Builds the relative path of node from cwd to node; if cwd is NULL, the
   absolute path is built (same as if cwd is the root). Path is malloc()'d
   and the caller is responsible for free()'ing it. Returns NULL if
   cwd is not NULL and node is not under cwd. */
char *lht_tree_path_build(lht_node_t *cwd, lht_node_t *node, lht_err_t *err);

/* checks whether a symlink is broken, relative to cwd. The document the
   check is done on is cwd's document if cwd is not NULL, or sy's document
   if cwd is NULL.
   Returns:
    1 if it is broken
    0 if it is valid (points to a node that can be retrieved by the symlink)
    -1 if sy is not a symlink.
*/
int lht_tree_symlink_is_broken(lht_node_t *cwd, lht_node_t *sy);


/* checks whether a subtree has symlinks; if chk_broken is non-zero, also
   check for broken symlinks. If tree itself is a symlink, it is not
   checked, only its children. Returns:
    0 there is no symlink in the subtree
    1 there is at least one symlink in the subtree and chk_broken == 0
    1 there is at least one broken symlink in the subtree and chk_broken != 0
*/
int lht_tree_has_symlink(lht_node_t *tree, int chk_broken);


/* merge src node into dst; if the merge would fail, no modification is made to
   dst or src or their children. If the merge is successful, src is and subnodes
   are deleted from their original tree and the pointer to src becomes invalid. */
lht_err_t lht_tree_merge(lht_node_t *dst, lht_node_t *src);
lht_err_t lht_tree_merge_using(lht_node_t *dst, lht_node_t *src, lht_err_t recurse(lht_node_t *, lht_node_t *));

/* low level default mergers */
void lht_tree_merge_text(lht_node_t *dst, lht_node_t *src);
void lht_tree_merge_list(lht_node_t *dst, lht_node_t *src);
lht_err_t lht_tree_merge_table(lht_node_t *dst, lht_node_t *src);
lht_err_t lht_tree_merge_hash(lht_node_t *dst, lht_node_t *src, lht_err_t recurse(lht_node_t *, lht_node_t *));


/* --- LIST --- */
/* returns the nth element of a list (slow), or NULL if n is
   out of bounds; n counts from 0 */
lht_node_t *lht_tree_list_nth(const lht_node_t *list, int n);

/* returns the nth name match on the list (e.g. "the 2nd node which is called foo";
   very slow), or NULL if n is out of bounds; n counts from 0 */
lht_node_t *lht_tree_list_nthname(const lht_node_t *list, int n, const char *name);

/* delete/detach the nth element of a list (slow); returns 0 on success; n counts from 0*/
lht_err_t lht_tree_list_del_nth(lht_node_t *list, int n);
lht_node_t *lht_tree_list_detach_nth(lht_node_t *list, int n);

/* delete/detach the node from a list (slow); returns 0 on success */
lht_err_t lht_tree_list_del_child(lht_node_t *list, lht_node_t *node);
lht_err_t lht_tree_list_detach_child(lht_node_t *list, lht_node_t *node);

/* replace node with (already detached) newn on a list (keeping order of nodes) */
lht_err_t lht_tree_list_replace_child(lht_node_t *list, lht_node_t *node, lht_node_t *newn);

/* linear search for node in a list; returns index if found, -1 if not found. */
int lht_tree_list_find_node(lht_node_t *list, lht_node_t *node);

/* remove and free all elements of a list */
void lht_tree_list_clean(lht_node_t *lst);

/* --- HASH --- */

/* returns the number of elements in the hash */
int lht_tree_hash_len(lht_node_t *list);

/* del/detach node from a hash (by ptr) */
lht_err_t lht_tree_hash_del_child(lht_node_t *hash, lht_node_t *node);
lht_err_t lht_tree_hash_detach_child(lht_node_t *hash, lht_node_t *node);

/* replace node with (already detached) newn - they may have different names */
lht_err_t lht_tree_hash_replace_child(lht_node_t *hash, lht_node_t *node, lht_node_t *newn);


/* --- TABLE --- */
/* all row and col arguments count from 0 */

/* insert a row before base_row; base_row may be (number-of-rows) to
   append at the end of the table. Cells are anon empty strings;
   returns LHTE_SUCCESS, LHTE_BOUNDARY or LHTE_OUT_OF_MEM */
lht_err_t lht_tree_table_ins_row(lht_node_t *table, int base_row);


/* insert a column before base_col; base_col may be (number-of-cols) to
   append at the end of the table. Cells are anon empty strings;
   returns LHTE_SUCCESS, LHTE_BOUNDARY or LHTE_OUT_OF_MEM */
lht_err_t lht_tree_table_ins_col(lht_node_t *table, int base_col);

/* delete the rth row of a table (freeing up all cells of that row);
   return LHTE_SUCCESS or LHTE_BOUNDARY */
lht_err_t lht_tree_table_del_row(lht_node_t *table, int r);

/* delete the cth col of a table (freeing up all cells of that col);
   returns LHTE_SUCCESS or LHTE_BOUNDARY */
lht_err_t lht_tree_table_del_col(lht_node_t *table, int c);

/* linear search for node in table; returns 1 if found, 0 if not found. When
   found row and col are set. */
int lht_tree_table_find_cell(lht_node_t *table, lht_node_t *node, int *row, int *col);

/* return the cell, that matches:
   - row_name != NULL: (row_num)th row that matches name row_name
   - row_name == NULL: (row_num)th row
   and
   - col_name != NULL: (col_num)th col that matches name col_name
   - col_name == NULL: (col_num)th col
   Return NULL if not found
*/
lht_node_t *lht_tree_table_named_cell(const lht_node_t *table, const char *row_name, int row_num, const char *col_name, int col_num);

/* detach a cell from a table by replacing it with the anonymous empty text node */
lht_node_t *lht_tree_table_detach_cell(lht_node_t *table, int row, int col, lht_err_t *err);
lht_err_t lht_tree_table_detach_child(lht_node_t *table, lht_node_t *node);


/* low level row allocator: leaves the cells and row-name uninitialized */
lht_err_t lht_tree_table_ins_row_(lht_node_t *table, int base_row);

/* low level child replace; newn should be detached */
lht_err_t lht_tree_table_replace_child(lht_node_t *table, lht_node_t *existing, lht_node_t *newn);
lht_node_t *lht_tree_table_replace_cell(lht_node_t *table, int row, int col, lht_err_t *err, lht_node_t *newn);


#endif
