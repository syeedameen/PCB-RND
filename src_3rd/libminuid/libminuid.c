/*
    libminuid - minimalistic globally unique IDs
    Copyright (C) 2017  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Project page:  http://repo.hu/projects/libminuid
    Source code:   svn://repo.hu/libminuid/trunk
    Author:        emailto: libminuid [at] igor2.repo.hu
*/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include "libminuid.h"
#include "base64.h"

int minuid_salt(minuid_session_t *sess, const void *salt, int salt_len)
{
	const char *s;

	if (salt_len <= 0)
		return -1;

	for(s = salt; salt_len > 0; s++, salt_len--) {
		sess->sid[sess->salt_offs] ^= *s;
		sess->salt_offs++;
		if (sess->salt_offs >= MINUID_SID_LEN)
			sess->salt_offs = 0;
	}

	return 0;
}

/* Salt from a file, typically a device file like /dev/urandom on *NIX */
static int try_file_salt(minuid_session_t *sess, char *fn)
{
	FILE *f;
	unsigned char buff[MINUID_SID_LEN];
	int len;

	f = fopen(fn, "rb");
	if (f == NULL)
		return 0;

	len = fread(buff, 1, MINUID_SID_LEN, f);
	fclose(f);

	if (len > 0)
		minuid_salt(sess, buff, len);

	return len > 2 * MINUID_SID_LEN / 3;
}


/* salt from pseudorandom */
static int try_time_salt(minuid_session_t *sess)
{
	time_t t = time(NULL);
	minuid_salt(sess, &t, sizeof(t));
	return 0;
}


/* Create a new session in sess */
int minuid_init(minuid_session_t *sess)
{
	memset(sess, 0, sizeof(minuid_session_t));
	(void)(try_file_salt(sess, "/dev/urandom") || try_file_salt(sess, "/dev/random") || try_time_salt(sess));
	return 0;
}

int minuid_gen(minuid_session_t *sess, minuid_bin_t dst)
{
	unsigned char *d;
	sess->seqno++;
	if (sess->seqno == 0) {
		/* overflow; bump sid */
		unsigned char one = 1;
		try_time_salt(sess);
		minuid_salt(sess, &one, 1); /* just in case time was 0 */
	}

	d = (unsigned char *)dst;
	memcpy(d, sess->sid, MINUID_SID_LEN);
	d += MINUID_SID_LEN;
	*d++ = ((sess->seqno & 0xFF000000UL) >> 24);
	*d++ = ((sess->seqno & 0x00FF0000UL) >> 16);
	*d++ = ((sess->seqno & 0x0000FF00UL) >> 8);
	*d++ = (sess->seqno & 0x000000FFUL);
	return 0;
}


int minuid_cmp(const minuid_bin_t u1, const minuid_bin_t u2)
{
	return memcmp(u1, u2, sizeof(minuid_bin_t));
}

void minuid_cpy(minuid_bin_t dst, const minuid_bin_t src)
{
	memcpy(dst, src, sizeof(minuid_bin_t));
}

int minuid_bin2str(minuid_str_t dst, const minuid_bin_t src)
{
	const unsigned char *s_begin = (const unsigned char *)src;
	const unsigned char *s_end = s_begin + sizeof(minuid_bin_t) - 1, *s;
	char *d = (char *)dst + sizeof(minuid_str_t) - 1;
	unsigned int dig, acc = 0, bits = 0;

	*d-- = '\0';

	for(s = s_end; (s >= s_begin) || (bits > 0); d--) {
		/* shift in the next 6 bits */
		if (bits < 6) {
			acc |= ((unsigned int)*s) << bits;
			bits += 8;
			s--;
		}
		dig = acc & 0x3F;
		acc >>= 6;
		bits -= 6;

		*d = MINUID_BASE64_I2C[dig];
	}

	return 0;
}

int minuid_str2bin(minuid_bin_t dst, const minuid_str_t src)
{
	unsigned char *d_begin = (unsigned char *)dst;
	unsigned char *d_end = d_begin + sizeof(minuid_bin_t) - 1, *d;
	const char *s = (const char *)src + sizeof(minuid_str_t) - 1;
	const char *s_begin = (const char *)src;
	unsigned int acc = 0, bits = 0;
	int dig;

	if (*s != '\0')
		return -1;
	s--;

	for(d = d_end; (s >= s_begin) || (bits > 0); d--) {
		while(bits < 8) {
			dig = MINUID_BASE64_C2I[(int)*s];
			if (dig < 0)
				return -1;
			acc |= ((unsigned int)(dig))<<bits;
			bits += 6;
			s--;
		}
		*d = acc & 0xFF;
		acc >>= 8;
		bits -= 8;
	}

	return -1;
}

