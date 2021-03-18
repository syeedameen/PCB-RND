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

#ifndef LIBMINUID_H
#define LIBMINUID_H

#define MINUID_VER_MAJOR 1
#define MINUID_VER_MINOR 0
#define MINUID_VER_PATCH 0

#define MINUID_SID_LEN 14
#define MINUID_BIN_LEN (MINUID_SID_LEN+4)
#define MINUID_TXT_LEN (MINUID_BIN_LEN*8/6+1)

typedef struct minuid_session_s {
	unsigned char sid[MINUID_SID_LEN];
	unsigned long seqno;
	int salt_offs;
} minuid_session_t;

/* Binary form: 18 unsigned 8 bit integers */
typedef unsigned char minuid_bin_t[MINUID_BIN_LEN];

/* string form: 24 bytes of text and a \0 */
typedef char minuid_str_t[MINUID_TXT_LEN];

/* All calls return 0 on success, -1 on error; these functions are not
   thread-safe: each thread should have its own session (and salt it with
   the thread-ID). */

/* Create a new session in sess */
int minuid_init(minuid_session_t *sess);

/* XOR a salt onto a session's sid, increasing entropy; use a salt that is
   relatively unique (e.g. process ID to avoid two processes started the
   same time having the same sid on a system without a good random source) */
int minuid_salt(minuid_session_t *sess, const void *salt, int salt_len);

/* Generate a new uid in dst */
int minuid_gen(minuid_session_t *sess, minuid_bin_t dst);

/* Compare two uids; return is the same as memcmp()'s */
int minuid_cmp(const minuid_bin_t u1, const minuid_bin_t u2);

/* Copy src uid to dst (dst shouldn't overlap with src) */
void minuid_cpy(minuid_bin_t dst, const minuid_bin_t src);


/* convert to/from string */
int minuid_bin2str(minuid_str_t dst, const minuid_bin_t src);
int minuid_str2bin(minuid_bin_t dst, const minuid_str_t src);

#endif
