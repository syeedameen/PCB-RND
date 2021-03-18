#define GVTTYPE GVT(t)

typedef struct GVT(s) GVTTYPE;


struct GVT(s) {
	GVT_SIZE_TYPE used, alloced;
	GVT_ELEM_TYPE *array;
#ifdef GVT_INIT_ELEM_FUNC
	void (*init_elem)(GVTTYPE *vect, GVT_ELEM_TYPE *elem);
#endif
#ifdef GVT_ELEM_CONSTRUCTOR
	int (*elem_constructor)(GVTTYPE *vect, GVT_ELEM_TYPE *elem);
#endif
#ifdef GVT_ELEM_DESTRUCTOR
	void (*elem_destructor)(GVTTYPE *vect, GVT_ELEM_TYPE *elem);
#endif
#ifdef GVT_ELEM_COPY
	int (*elem_copy)(GVTTYPE *dst_vect, GVT_ELEM_TYPE *dst, const GVT_ELEM_TYPE *src);
#endif
#ifdef GVT_USER_FIELDS
	GVT_USER_FIELDS
#endif
#ifdef GVT_OPTIONAL_NO_REALLOC
	char no_realloc;
#endif
};

/* Append is possible only if we can determine the length of the input string,
   either because there's an existing strlen() or because we can count
   characters looking for a terminator */
#ifdef GVT_STRLEN
#	define GVT_NEED_APPEND_ARRAY
#else
#	ifdef GVT_TERM
#		define GVT_NEED_APPEND_ARRAY
#	endif
#endif

GVT_FUNC void GVT(init)(GVTTYPE *vect);
GVT_FUNC int GVT(uninit)(GVTTYPE *vect);

GVT_FUNC GVT_SIZE_TYPE GVT(len)(GVTTYPE *vect);
GVT_FUNC int GVT(in_bound)(GVTTYPE *vect, GVT_SIZE_TYPE idx);

GVT_FUNC GVT_ELEM_TYPE *GVT(get)(GVTTYPE *vect, GVT_SIZE_TYPE idx, int alloc);
GVT_FUNC int GVT(set)(GVTTYPE *vect, GVT_SIZE_TYPE idx, GVT_ELEM_TYPE src);
GVT_FUNC int GVT(set_ptr)(GVTTYPE *vect, GVT_SIZE_TYPE idx, GVT_ELEM_TYPE *src);

GVT_FUNC int GVT(resize)(GVTTYPE *vect, GVT_SIZE_TYPE new_size);
GVT_FUNC int GVT(truncate)(GVTTYPE *vect, GVT_SIZE_TYPE len);
GVT_FUNC int GVT(enlarge)(GVTTYPE *vect, GVT_SIZE_TYPE idx);

GVT_FUNC int GVT(append)(GVTTYPE *vect, GVT_ELEM_TYPE src);
GVT_FUNC int GVT(append_len)(GVTTYPE *vect, const GVT_ELEM_TYPE *src, GVT_SIZE_TYPE len);
GVT_FUNC int GVT(concat)(GVTTYPE *dst, const GVTTYPE *src);
GVT_FUNC int GVT(copy)(GVTTYPE *dst_vect, GVT_SIZE_TYPE dst_idx, GVTTYPE *src_vect, GVT_SIZE_TYPE src_idx, GVT_SIZE_TYPE num_elems);
GVT_FUNC GVT_ELEM_TYPE *GVT(alloc_append)(GVTTYPE *vect, int num_elems);

GVT_FUNC int GVT(remove)(GVTTYPE *vect, GVT_SIZE_TYPE from_idx, GVT_SIZE_TYPE num_elems);
GVT_FUNC int GVT(remove_bw)(GVTTYPE *vect, GVT_SIZE_TYPE to_idx, GVT_SIZE_TYPE num_elems);

/* Insert num_elems _before_ from_idx */
GVT_FUNC GVT_ELEM_TYPE *GVT(alloc_insert)(GVTTYPE *vect, GVT_SIZE_TYPE from_idx, GVT_SIZE_TYPE num_elems);

/* Insert src/len into vect at from_idx; enlarges vect by len. Returns pointer
   to from_idx. */
GVT_FUNC GVT_ELEM_TYPE *GVT(insert_len)(GVTTYPE *vect, GVT_SIZE_TYPE from_idx, GVT_ELEM_TYPE *src, GVT_SIZE_TYPE len);


#ifdef GVT_NEED_APPEND_ARRAY
GVT_FUNC int GVT(append_array)(GVTTYPE *vect, const GVT_ELEM_TYPE *src);
#endif


