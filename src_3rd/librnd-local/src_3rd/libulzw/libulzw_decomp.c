/*//////////////////////////////////////////////////////////////////////////
//                            **** LIBULZW ****                           //
//               Adjusted Binary LZW Compressor/Decompressor              //
//                     Copyright (c) 2016 David Bryant                    //
//                           All Rights Reserved                          //
//      Distributed under the BSD Software License (see license.txt)      //
//////////////////////////////////////////////////////////////////////////*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ULZW_CODES_DEFINED
#define ULZW_CODES_DEFINED 1
#define ULZW_NULL_CODE       -1  /* indicates a NULL prefix */
#define ULZW_CLEAR_CODE      256 /* code to flush dictionary and restart decoder */
#define ULZW_FIRST_STRING    257 /* code of first dictionary string */
#endif

/* LZW decompression function. Bytes (8-bit) are read and written through callbacks. The
 * "maxbits" parameter is read as the first byte in the stream and controls how much memory
 * is allocated for decoding. A return value of EOF from the "src" callback terminates the
 * compression process (although this should not normally occur). A non-zero return value
 * indicates an error, which in this case can be a bad "maxbits" read from the stream, a
 * failed malloc(), or if an EOF is read from the input stream before the compression
 * terminates naturally with END_CODE.
 */

int ulzw_decompress(void *ctx, void (*dst)(void *, int), int(*src)(void *))
{
	int read_byte, next = ULZW_FIRST_STRING, prefix = ULZW_CLEAR_CODE, bits = 0, total_codes;
	unsigned char *terminators, *reverse_buffer;
	unsigned long shifter = 0;
	short *prefixes;

	if ((read_byte = ((*src) (ctx))) == EOF || (read_byte & 0xfc)) /*sanitize first byte */
		return 1;

	/* based on the "maxbits" parameter, compute total codes and allocate dictionary storage */

	total_codes = 512 << (read_byte & 0x3);
	reverse_buffer = malloc((total_codes - 256) * sizeof(reverse_buffer[0]));
	prefixes = malloc((total_codes - 256) * sizeof(prefixes[0]));
	terminators = malloc((total_codes - 256) * sizeof(terminators[0]));

	if (!reverse_buffer || !prefixes || !terminators) /* check for mallco() failure */
		return 1;

	/* This is the main loop where we read input symbols. The values range from 0 to the code value
	   of the "next" string in the dictionary (although the actual "next" code cannot be used yet,
	   and so we reserve that code for the END_CODE). Note that receiving an EOF from the input
	   stream is actually an error because we should have gotten the END_CODE first. */

	while(1) {
		int code_bits = next < 1024 ? (next < 512 ? 8 : 9) : (next < 2048 ? 10 : 11), code;
		int extras = (1 << (code_bits + 1)) - next - 1;

		do {
			if ((read_byte = ((*src) (ctx))) == EOF) {
				free(terminators);
				free(prefixes);
				free(reverse_buffer);
				return 1;
			}

			shifter |= (long)read_byte << bits;
		} while((bits += 8) < code_bits);

		/* first we assume the code will fit in the minimum number of required bits */

		code = (int)shifter & ((1 << code_bits) - 1);
		shifter >>= code_bits;
		bits -= code_bits;

		/* but if code >= extras, then we need to read another bit to calculate the real code
		   (this is the "adjusted binary" part) */

		if (code >= extras) {
			if (!bits) {
				if ((read_byte = ((*src) (ctx))) == EOF) {
					free(terminators);
					free(prefixes);
					free(reverse_buffer);
					return 1;
				}

				shifter = (long)read_byte;
				bits = 8;
			}

			code = (code << 1) - extras + (shifter & 1);
			shifter >>= 1;
			bits--;
		}

		if (code == next) /* sending the maximum code is reserved for the end of the file */
			break;
		else if (code == ULZW_CLEAR_CODE) /* otherwise check for a ULZW_CLEAR_CODE to start over early */
			next = ULZW_FIRST_STRING;
		else if (prefix == ULZW_CLEAR_CODE) { /* this only happens at the first symbol which is always sent */
			(*dst) (ctx, code); /* literally and becomes our initial prefix */
			next++;
		}
		/* Otherwise we have a valid prefix so we step through the string from end to beginning storing the
		   bytes in the "reverse_buffer", and then we send them out in the proper order. One corner-case
		   we have to handle here is that the string might be the same one that is actually being defined
		   now (code == next-1). Also, the first 256 entries of "terminators" and "prefixes" are fixed and
		   not allocated, so that messes things up a bit. */
		else {
			int cti = (code == next - 1) ? prefix : code;
			unsigned char *rbp = reverse_buffer, c;

			do
				*rbp++ = cti < 256 ? cti : terminators[cti - 256]; /* step backward through string... */
			while((cti = (cti < 256) ? ULZW_NULL_CODE : prefixes[cti - 256]) != ULZW_NULL_CODE);

			c = *--rbp; /* the first byte in this string is the terminator for the last string, which is */
			/* the one that we'll create a new dictionary entry for this time */

			do
				(*dst) (ctx, *rbp); /* send string in corrected order (except for the terminator */
			while(rbp-- != reverse_buffer); /* which we don't know yet) */

			if (code == next - 1)
				(*dst) (ctx, c);

			prefixes[next - 1 - 256] = prefix; /* now update the next dictionary entry with the new string */
			terminators[next - 1 - 256] = c; /* (but we're always one behind, so it's not the string just sent) */

			if (++next == total_codes) /* check for full dictionary, which forces a reset (and, BTW, */
				next = ULZW_FIRST_STRING; /* means we'll never use the dictionary entry we just wrote) */
		}

		prefix = code; /* the code we just received becomes the prefix for the next dictionary string entry */
		/* (which we'll create once we find out the terminator) */
	}

	free(terminators);
	free(prefixes);
	free(reverse_buffer);
	return 0;
}
