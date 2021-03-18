/* API intended for internal use only - calls with side effects, assumptions
   or other dangerous features. */

#define LHT_TABLE_GROW_ROWS 16
#define LHT_TABLE_GROW_COLS 16

/* node type specific initializers */
void lht_dom_hash_init(lht_node_t *hash);

/* return the prefix for indentation without growing it */
typedef struct lht_dom_indent_s lht_dom_indent_t;
const char *lht_dom_indent_get(lht_dom_indent_t *ind, int level);

/* node type specific subtree print */
typedef void (*lht_dom_ptcb)(lht_node_t *node, FILE *outf, lht_dom_indent_t *ind_, int level);
void lht_dom_phash(lht_dom_ptcb ptree, lht_node_t *node, FILE *outf, lht_dom_indent_t *ind_, int level);
void lht_dom_plist(lht_dom_ptcb ptree, lht_node_t *node, FILE *outf, lht_dom_indent_t *ind_, int level);
void lht_dom_ptable(lht_dom_ptcb ptree, lht_node_t *node, FILE *outf, lht_dom_indent_t *ind_, int level);

void lht_dom_flist(lht_node_t *node);
void lht_dom_ftable(lht_node_t *node);
void lht_dom_fhash(lht_node_t *node);

lht_err_t lht_dom_table_post_process(lht_node_t *dest, lht_node_t *row_list);

lht_node_t *lht_dom_list_first(lht_dom_iterator_t *it, lht_node_t *parent);
lht_node_t *lht_dom_list_next(lht_dom_iterator_t *it);
lht_node_t *lht_dom_hash_first(lht_dom_iterator_t *it, lht_node_t *parent);
lht_node_t *lht_dom_hash_next(lht_dom_iterator_t *it);
lht_node_t *lht_dom_table_first(lht_dom_iterator_t *it, lht_node_t *parent);
lht_node_t *lht_dom_table_next(lht_dom_iterator_t *it);

/* detach node from its current document and move it into a newly allocated document */
lht_err_t lht_dom_loc_detach(lht_node_t *node);

/* table grow */
lht_err_t lht_dom_table_grow_row(lht_node_t *table);
lht_err_t lht_dom_table_grow_col(lht_node_t *table);

/* move a node recursively from its current doc into tdoc. This will affect
   the following lht_node_t fields:
    - file_name (content won't change but the pointer will)
    - doc (will point to tdoc)
   Furthermore all file names found during the move will be introduced in
   the location file name hash of tdoc, but will not deleted from the original
   doc. The node is _not_ detached/unlinked from its parent!

   Shall be called after detaching/unlinking the node.
*/
int lht_dom_loc_move(lht_doc_t *tdoc, lht_node_t *node);
