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

/* This macro writes the adjusted-binary symbol "code" given the maximum
 * symbol "maxcode". A macro is used here just to avoid the duplication in
 * the ulzw_compress() function. The idea is that if "maxcode" is not one
 * less than a power of two (which it rarely will be) then this code can
 * often send fewer bits that would be required with a fixed-sized code.
 *
 * For example, the first code we send will have a "maxcode" of 257, so
 * every "code" would normally consume 9 bits. But with adjusted binary we
 * can actually represent any code from 0 to 253 with just 8 bits -- only
 * the 4 codes from 254 to 257 take 9 bits.
 */

#define ULZW_WRITE_CODE(code,maxcode) do {                           \
    int code_bits = (maxcode) < 1024 ?                          \
        ((maxcode) < 512 ? 8 : 9) :                             \
        ((maxcode) < 2048 ? 10 : 11);                           \
    int extras = (1 << (code_bits + 1)) - (maxcode) - 1;        \
    if ((code) < extras) {                                      \
        shifter |= ((long)(code) << bits);                      \
        bits += code_bits;                                      \
    }                                                           \
    else {                                                      \
        shifter |= ((long)(((code) + extras) >> 1) << bits);    \
        bits += code_bits;                                      \
        shifter |= ((long)(((code) + extras) & 1) << bits++);   \
    }                                                           \
    do { (*dst)(ctx, shifter); shifter >>= 8; output_bytes++;   \
    } while ((bits -= 8) >= 8);                                 \
} while (0)

/* LZW compression function. Bytes (8-bit) are read and written through callbacks and the
 * "maxbits" parameter specifies the maximum symbol size (9-12), which in turn determines
 * the RAM requirement and, to a large extent, the level of compression achievable. A return
 * value of EOF from the "src" callback terminates the compression process. A non-zero return
 * value indicates one of the two possible errors -- bad "maxbits" param or failed malloc().
 */

int ulzw_compress(void *ctx, void (*dst)(void *, int), int(*src)(void *), int maxbits)
{
	int next = ULZW_FIRST_STRING, prefix = ULZW_NULL_CODE, bits = 0, total_codes, c;
	unsigned long input_bytes = 0, output_bytes = 0;
	short *first_references, *next_references;
	unsigned char *terminators;
	unsigned long shifter = 0;

	if (maxbits < 9 || maxbits > 12) /* check for valid "maxbits" setting */
		return 1;

	/* based on the "maxbits" parameter, compute total codes and allocate dictionary storage */

	total_codes = 1 << maxbits;
	first_references = malloc(total_codes * sizeof(first_references[0]));
	next_references = malloc((total_codes - 256) * sizeof(next_references[0]));
	terminators = malloc((total_codes - 256) * sizeof(terminators[0]));

	if (!first_references || !next_references || !terminators)
		return 1; /* failed malloc() */

	/* clear the dictionary */

	memset(first_references, 0, total_codes * sizeof(first_references[0]));
	memset(next_references, 0, (total_codes - 256) * sizeof(next_references[0]));
	memset(terminators, 0, (total_codes - 256) * sizeof(terminators[0]));

	(*dst) (ctx, maxbits - 9);	/* first byte in output stream indicates the maximum symbol bits */

	/* This is the main loop where we read input bytes and compress them. We always keep track of the
	   "prefix", which represents a pending byte (if < 256) or string entry (if >= ULZW_FIRST_STRING) that
	   has not been sent to the decoder yet. The output symbols are kept in the "shifter" and "bits"
	   variables and are sent to the output every time 8 bits are available (done in the macro). */

	while((c = (*src) (ctx)) != EOF) {
		int cti; /* coding table index */

		input_bytes++;

		if (prefix == ULZW_NULL_CODE) { /* this only happens the very first byte when we don't yet have a prefix */
			prefix = c;
			continue;
		}

		if ((cti = first_references[prefix])) { /* if any longer strings are built on the current prefix... */
			while(1)
				if (terminators[cti - 256] == c) { /* we found a matching string, so we just update the prefix */
					prefix = cti; /* to that string and continue without sending anything */
					break;
				}
				else if (!next_references[cti - 256]) { /* this string did not match the new character and */
					next_references[cti - 256] = next; /* there aren't any more, so we'll add a new string */
					cti = 0; /* and point to it with "next_reference" */
					break;
				}
				else
					cti = next_references[cti - 256]; /* there are more possible matches to check, so loop back */
		}
		else /* no longer strings are based on the current prefix, so now */
			first_references[prefix] = next; /* the current prefix plus the new byte will be the next string */

		/* If "cti" is zero, we could not simply extend our "prefix" to a longer string because we did not find a
		   dictionary match, so we send the symbol representing the current "prefix" and add the new string to the
		   dictionary. Since the current byte "c" was not included in the prefix, that now becomes our new prefix. */

		if (!cti) {
			ULZW_WRITE_CODE(prefix, next); /* send symbol for current prefix (0 to next-1) */
			terminators[next - 256] = c; /* newly created string has current byte as the terminator */
			prefix = c; /* current byte also becomes new prefix for next string */

			/* This is where we bump the next string index and decide whether to clear the dictionary and start over.
			   The triggers for that are either the dictionary is full or we've been outputting too many bytes and
			   decide to cut our losses before the symbols get any larger. Note that for the dictionary full case we
			   do NOT send the ULZW_CLEAR_CODE because the decoder knows about this and we don't want to be redundant. */

			if (++next == total_codes || output_bytes > 8 + input_bytes + (input_bytes >> 4)) {
				if (next < total_codes)
					ULZW_WRITE_CODE(ULZW_CLEAR_CODE, next);

				/* clear the dictionary and reset the byte counters -- basically everything starts over
				   except that we keep the last pending "prefix" (which, of course, was never sent) */

				memset(first_references, 0, total_codes * sizeof(first_references[0]));
				memset(next_references, 0, (total_codes - 256) * sizeof(next_references[0]));
				memset(terminators, 0, (total_codes - 256) * sizeof(terminators[0]));
				input_bytes = output_bytes = 0;
				next = ULZW_FIRST_STRING;
			}
		}
	}

	/* we're done with input, so if we've received anything we still need to send that pesky pending prefix... */

	if (prefix != ULZW_NULL_CODE) {
		ULZW_WRITE_CODE(prefix, next);

		if (++next == total_codes) /* watch for clearing to the first string to stay in step with the decoder! */
			next = ULZW_FIRST_STRING; /* (this was actually a corner-case bug that did not trigger often) */
	}

	ULZW_WRITE_CODE(next, next); /* the maximum possible code is always reserved for our END_CODE */

	if (bits) /* finally, flush any pending bits from the shifter */
		(*dst) (ctx, shifter);

	free(terminators);
	free(next_references);
	free(first_references);
	return 0;
}
