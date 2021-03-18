/*
Copyright (c) 2016 Tibor 'Igor2' Palinkas
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of the Author nor the names of contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

Contact: genvector {at} igor2.repo.hu
Project page: http://repo.hu/projects/genvector
*/

#ifndef GVT_TERMSIZE
#	ifdef GVT_TERM
#		define GVT_TERMSIZE sizeof(GVT_TERM)
#	else
#		define GVT_TERMSIZE 0
#	endif
#endif

#ifndef GVT_FUNC
#	define GVT_FUNC
#endif

GVT_FUNC void GVT(init)(GVTTYPE *vect)
{
	vect->used = vect->alloced = 0;
	vect->array = NULL;

#ifdef GVT_OPTIONAL_NO_REALLOC
	vect->no_realloc = 0;
#endif

#ifdef GVT_TERM
	/* If we have a temrinator, an initialized array is zero length, but allocated
	   and terminated. */
	GVT(resize)(vect, 1);
	vect->array[0] = GVT_TERM;
#endif
}

GVT_FUNC GVT_SIZE_TYPE GVT(len)(GVTTYPE *vect)
{
	return vect->used;
}


static void GVT(init_block)(GVTTYPE *vect, GVT_ELEM_TYPE *arr, GVT_SIZE_TYPE from, GVT_SIZE_TYPE to)
{
#ifdef GVT_SET_NEW_BYTES_TO
	memset(arr+from, GVT_SET_NEW_BYTES_TO, (to-from) * sizeof(GVT_ELEM_TYPE));
#endif
#ifdef GVT_INIT_ELEM_FUNC
	if (vect->init_elem != NULL) {
		GVT_SIZE_TYPE n;
		for(n = from; n < to; n++)
			vect->init_elem(vect, arr+n);
	}
#endif
}

static int GVT(construct_block)(GVTTYPE *vect, GVT_ELEM_TYPE *arr, GVT_SIZE_TYPE from, GVT_SIZE_TYPE to)
{
	int res = 0;
#ifdef GVT_ELEM_CONSTRUCTOR
	if (vect->elem_constructor != NULL) {
		GVT_SIZE_TYPE n;
		for(n = from; n < to; n++)
			res |= vect->elem_constructor(vect, arr+n);
	}
#endif
	return res;
}

static void GVT(destruct_block)(GVTTYPE *vect, GVT_ELEM_TYPE *arr, GVT_SIZE_TYPE from, GVT_SIZE_TYPE to)
{
#ifdef GVT_ELEM_CONSTRUCTOR
	if (vect->elem_destructor != NULL) {
		GVT_SIZE_TYPE n;
		for(n = from; n < to; n++)
			vect->elem_destructor(vect, arr+n);
	}
#endif
}

GVT_FUNC int GVT(resize)(GVTTYPE *vect, GVT_SIZE_TYPE new_size)
{
	GVT_SIZE_TYPE nall;
	GVT_ELEM_TYPE *narr;

#ifdef GVT_OPTIONAL_NO_REALLOC
	if (vect->no_realloc) {
		if (new_size <= vect->alloced) {
			if (new_size < vect->used)
				vect->used = new_size;
			return 0;
		}
		return -1;
	}
#endif

	if (new_size == 0) {
		GVT(destruct_block)(vect, vect->array, 0, vect->used);
		GVT_FREE(vect, vect->array);
		vect->alloced = vect->used = 0;
		vect->array = NULL;
		return 0;
	}

	if (new_size > vect->alloced) {
		/* grow */
		if (new_size < GVT_DOUBLING_THRS) {
			nall = vect->alloced;
			if (nall < GVT_START_SIZE)
				nall = GVT_START_SIZE;
			while(nall < new_size)
				nall *= 2;
		}
		else
			nall = new_size + GVT_START_SIZE;
	}
	else if (new_size < vect->used) {
		/* shrink */
		GVT(destruct_block)(vect, vect->array, new_size, vect->used);

		nall = new_size;
		if (nall < GVT_START_SIZE)
			nall = GVT_START_SIZE;
	}
	else {
		/* no need to resize */
		return 0;
	}

	narr = GVT_REALLOC(vect, vect->array, nall * sizeof(GVT_ELEM_TYPE));
	if (!(new_size > vect->alloced)) {
		/* allocation failed for grow: error */
		if (narr == NULL) 
			return -1;
	}
	else {
		/* allocation failed for shrink: very unlikely, but we can live with it, just keep the original pointer */
		if (narr == NULL) 
			narr = vect->array;
	}

	if (new_size > vect->alloced)
		GVT(init_block)(vect, narr, vect->used, nall);

	vect->alloced = nall;
	vect->array = narr;
	if (new_size < vect->used)
		vect->used = new_size;

	return 0;
}

GVT_FUNC int GVT(uninit)(GVTTYPE *vect)
{
	return GVT(resize)(vect, 0);
}

static int GVT(copy_block)(GVTTYPE *vect, GVT_ELEM_TYPE *dst, const GVT_ELEM_TYPE *src, GVT_SIZE_TYPE len)
{
#ifdef GVT_ELEM_COPY
	if (vect->elem_copy != NULL) {
		for(; len > 0; len--) {
			if (vect->elem_copy(vect, dst, src) != 0)
				return -1;
			dst++;
			src++;
		}
		return 0;
	}
#endif
	memcpy(dst, src, len * sizeof(GVT_ELEM_TYPE));
	return 0;
}

GVT_FUNC int GVT(append_len)(GVTTYPE *vect, const GVT_ELEM_TYPE *src, GVT_SIZE_TYPE len)
{
	int res;

	if (len == 0)
		return 0;

	res = GVT(resize)(vect, vect->used + len + GVT_TERMSIZE);
	if (res == 0) {
		if (GVT(copy_block)(vect, vect->array + vect->used, src, len) != 0)
			return -1;
/*		GVT(construct_block)(vect, vect->array, vect->used, vect->used+len);*/
		vect->used += len;
#ifdef GVT_TERM
		vect->array[vect->used] = GVT_TERM;
#endif
	}
	return res;
}

GVT_FUNC int GVT(append)(GVTTYPE *vect, GVT_ELEM_TYPE src)
{
	return GVT(append_len)(vect, &src, 1);
}

GVT_FUNC GVT_ELEM_TYPE *GVT(alloc_append)(GVTTYPE *vect, int num_elems)
{
	GVT_ELEM_TYPE *ret = NULL;
	int res;

	res = GVT(resize)(vect, vect->used + num_elems + GVT_TERMSIZE);
	if (res == 0) {
		GVT(init_block)(vect, vect->array, vect->used, vect->used+num_elems);
		if (GVT(construct_block)(vect, vect->array, vect->used, vect->used+num_elems) == 0) {
			ret = vect->array + vect->used;
			vect->used += num_elems;
		}
#ifdef GVT_TERM
		vect->array[vect->used] = GVT_TERM;
#endif
	}
	return ret;
}

#ifdef GVT_NEED_APPEND_ARRAY
GVT_FUNC int GVT(append_array)(GVTTYPE *vect, const GVT_ELEM_TYPE *src)
{
	GVT_SIZE_TYPE len;

#ifdef GVT_STRLEN
	len = GVT_STRLEN(src);
#else
	{
		const GVT_ELEM_TYPE *s;
		len = 0;
		for(s = src; *s != GVT_TERM; s++)
			len++;
	}
#endif

	return GVT(append_len)(vect, src, len);
}
#undef GVT_NEED_APPEND_ARRAY
#endif

GVT_FUNC int GVT(truncate)(GVTTYPE *vect, GVT_SIZE_TYPE len)
{
	int res;

	if (len > vect->used)
		return -1;
	if (len == vect->used)
		return 0;

	res = GVT(resize)(vect, len + GVT_TERMSIZE);
	if (res != 0)
		return -1;

	vect->used = len;
#ifdef GVT_TERM
		vect->array[len] = GVT_TERM;
#endif
	return 0;
}


GVT_FUNC int GVT(concat)(GVTTYPE *dst, const GVTTYPE *src)
{
	return GVT(append_len)(dst, src->array, src->used);
}

GVT_FUNC int GVT(in_bound)(GVTTYPE *vect, GVT_SIZE_TYPE idx)
{
	return idx < vect->used;
}

static int GVT(enlarge_)(GVTTYPE *vect, GVT_SIZE_TYPE idx, GVT_SIZE_TYPE init_idx)
{
	GVT_SIZE_TYPE max = vect->alloced;
	if (idx+GVT_TERMSIZE < vect->used)
		return 0;
	if (idx+GVT_TERMSIZE >= vect->alloced) {
		if (GVT(resize)(vect, idx+GVT_TERMSIZE+1) != 0)
			return -1;
	}

	if (init_idx+1 < max)
		max = init_idx+1;
	GVT(init_block)(vect, vect->array, vect->used, max);
	if (GVT(construct_block)(vect, vect->array, vect->used, init_idx+1) != 0)
		return -1;
	vect->used = idx+1;
#ifdef GVT_TERM
	vect->array[idx+1] = GVT_TERM;
#endif
	return 0;
}

GVT_FUNC int GVT(enlarge)(GVTTYPE *vect, GVT_SIZE_TYPE idx)
{
	return GVT(enlarge_)(vect, idx, idx);
}


GVT_FUNC int GVT(set_ptr)(GVTTYPE *vect, GVT_SIZE_TYPE idx, GVT_ELEM_TYPE *src)
{
#ifdef GVT_ELEM_CONSTRUCTOR
	GVT_SIZE_TYPE old_used = vect->used;
#endif

	if (idx+GVT_TERMSIZE >= vect->used) {
		/* enlarge if needed, do not initialize the elem we are going to set */
		if (GVT(enlarge_)(vect, idx, idx-1) != 0)
			return -1;
	}
	else {
		/* no need to enlarge - destruct the elem we are going to overwrite */
#ifdef GVT_ELEM_DESTRUCTOR
		if (vect->elem_destructor != NULL)
			vect->elem_destructor(vect, vect->array+idx);
#endif
	}

#ifdef GVT_ELEM_COPY
	if (vect->elem_copy != NULL) {
		if (vect->elem_copy(vect, &(vect->array[idx]), src) != 0) {
			vect->used = old_used;
			return -1;
		}
		return 0;
	}
#endif

	vect->array[idx] = *src;
	return 0;
}

GVT_FUNC int GVT(set)(GVTTYPE *vect, GVT_SIZE_TYPE idx, GVT_ELEM_TYPE src)
{
	return GVT(set_ptr)(vect, idx, &src);
}

GVT_FUNC GVT_ELEM_TYPE *GVT(get)(GVTTYPE *vect, GVT_SIZE_TYPE idx, int alloc)
{
	if (idx >= vect->used) {
		if (!alloc)
			return NULL;
	}
	if (GVT(enlarge_)(vect, idx, idx) != 0)
		return NULL;
	return &(vect->array[idx]);
}

GVT_FUNC int GVT(remove)(GVTTYPE *vect, GVT_SIZE_TYPE from_idx, GVT_SIZE_TYPE num_elems)
{
	GVT_SIZE_TYPE len;

	/* can't start deleting beyond the end */
	if (from_idx >= vect->used)
		return -1;

	/* accept deletion longer than the array but stop at the end */
	if (from_idx+num_elems > vect->used)
		num_elems = vect->used - from_idx;

	/* if we end up trying to delete 0 elements, we are done */
	if (num_elems == 0)
		return 0;

	GVT(destruct_block)(vect, vect->array, from_idx, from_idx+num_elems);
	len = vect->used - (from_idx+num_elems);
	if (len != 0)
		memmove(vect->array+from_idx, vect->array+from_idx+num_elems, (len) * sizeof(GVT_ELEM_TYPE));
	vect->used -= num_elems;

#ifdef GVT_TERM
	vect->array[vect->used] = GVT_TERM;
#endif
	return GVT(resize)(vect, vect->used + GVT_TERMSIZE);
}

GVT_FUNC int GVT(remove_bw)(GVTTYPE *vect, GVT_SIZE_TYPE to_idx, GVT_SIZE_TYPE num_elems)
{
	/* do not attempt to start deletion beyond the end of the array */
	if (to_idx >= vect->used)
		return -1;

	if (to_idx < num_elems)
		return GVT(remove)(vect, 0, to_idx+1);
	return GVT(remove)(vect, to_idx - num_elems + 1, num_elems);
}

GVT_FUNC GVT_ELEM_TYPE *GVT(alloc_insert)(GVTTYPE *vect, GVT_SIZE_TYPE from_idx, GVT_SIZE_TYPE num_elems)
{
	GVT_SIZE_TYPE len = vect->used - from_idx;
	GVT(enlarge)(vect, vect->used+num_elems-1);
	memmove(vect->array+from_idx+num_elems, vect->array+from_idx, (len) * sizeof(GVT_ELEM_TYPE));
	GVT(init_block)(vect, vect->array, from_idx, from_idx+num_elems-1);
	return &vect->array[from_idx];
}

GVT_FUNC int GVT(copy)(GVTTYPE *dst_vect, GVT_SIZE_TYPE dst_idx, GVTTYPE *src_vect, GVT_SIZE_TYPE src_idx, GVT_SIZE_TYPE num_elems)
{
	GVT_SIZE_TYPE newsize;

	/* src can't start beyond the array */
	if (src_idx >= src_vect->used)
		return -1;

	/* nop-copy */
	if ((dst_vect->array == src_vect->array) && (dst_idx == src_idx))
		return 0;

	/* truncate num elems at the end of src */
	if ((src_idx+num_elems-1) >= src_vect->used)
		num_elems = src_vect->used - src_idx;

	/* copying 0 elements is easy */
	if (num_elems == 0)
		return 0;

	newsize = dst_idx+num_elems-1;

	/* make sure destination vector is large enough */
	if (newsize >= dst_vect->used) {
		if (GVT(resize)(dst_vect, newsize+GVT_TERMSIZE+1) != 0)
			return -1;
#ifdef GVT_TERM
		dst_vect->array[newsize+1] = GVT_TERM;
#endif
	}

	/* if dst_idx is beyond dst's used, there will be some emelents that need
	   implicit initialization */
	if (dst_idx > dst_vect->used) {
		GVT(init_block)(dst_vect, dst_vect->array, dst_vect->used, dst_idx);
		if (GVT(construct_block)(dst_vect, dst_vect->array, dst_vect->used, dst_idx) != 0)
			return -1;
	}


#ifdef GVT_ELEM_COPY
	/* the slow way: have to copy element by element */
	if (dst_vect->elem_copy != NULL) {
		GVT_SIZE_TYPE n;

		if ((dst_vect->array == src_vect->array) && (dst_idx > src_idx)) {
			/* Backward copy to avoid losing elements in the overlap */
			for(n = num_elems; n > 0; n--) {
#ifdef GVT_ELEM_DESTRUCTOR
				if ((dst_vect->elem_destructor != NULL) && (n+dst_idx-1 < dst_vect->used))
					dst_vect->elem_destructor(dst_vect, dst_vect->array+n+dst_idx-1);
#endif
				if (dst_vect->elem_copy(dst_vect, &(dst_vect->array[n+dst_idx-1]), &(src_vect->array[n+src_idx-1])) != 0)
					return -1;
			}
		}
		else {
			/* Normal forward copy */
			for(n = 0; n < num_elems; n++) {
#ifdef GVT_ELEM_DESTRUCTOR
				if ((dst_vect->elem_destructor != NULL) && (n+dst_idx < dst_vect->used))
					dst_vect->elem_destructor(dst_vect, dst_vect->array+n+dst_idx);
#endif
				if (dst_vect->elem_copy(dst_vect, &(dst_vect->array[n+dst_idx]), &(src_vect->array[n+src_idx])) != 0)
					return -1;
			}
		}


		goto update_used;
	}
#endif

#ifdef GVT_ELEM_DESTRUCTOR
	/* need to destruct elements that are goung to be overwritten */
	if (dst_vect->elem_destructor != NULL) {
		GVT_SIZE_TYPE n;
		for(n = dst_idx; (n < num_elems) && (n < dst_vect->used); n++)
			dst_vect->elem_destructor(dst_vect, dst_vect->array+n);
	}
#endif

	/* the fast way: use memmove() */
	memmove(&(dst_vect->array[dst_idx]), &(src_vect->array[src_idx]), num_elems * sizeof(GVT_ELEM_TYPE));

#ifdef GVT_ELEM_COPY
	update_used:;
#endif
	if (dst_idx+num_elems > dst_vect->used)
		dst_vect->used = dst_idx+num_elems;
	return 0;
}

GVT_FUNC GVT_ELEM_TYPE *GVT(insert_len)(GVTTYPE *vect, GVT_SIZE_TYPE from_idx, GVT_ELEM_TYPE *src, GVT_SIZE_TYPE len)
{
	GVT_ELEM_TYPE *dst = GVT(alloc_insert)(vect, from_idx, len);
	memcpy(dst, src, len * sizeof(GVT_ELEM_TYPE));
	return dst;
}
