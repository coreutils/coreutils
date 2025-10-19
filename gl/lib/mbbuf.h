/* Buffering for multi-byte characters.
   Copyright (C) 2025 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Collin Funk.  */

#ifndef _MBBUF_H
#define _MBBUF_H 1

#ifndef _GL_INLINE_HEADER_BEGIN
# error "Please include config.h first."
#endif

#include <stdio.h>
#include <stddef.h>

#include "mcel.h"
#include "idx.h"

_GL_INLINE_HEADER_BEGIN
#ifndef MBBUF_INLINE
# define MBBUF_INLINE _GL_INLINE
#endif

/* End of file.  */
#define MBBUF_EOF UINT32_MAX

/* MBBUF_EOF should not be a valid character.  */
static_assert (MCEL_CHAR_MAX < MBBUF_EOF);

typedef struct
{
  char *buffer;    /* Input buffer.  */
  FILE *fp;        /* Input file stream.  */
  idx_t size;      /* Number of bytes allocated for BUFFER.  */
  idx_t length;    /* Number of bytes with data in BUFFER.  */
  idx_t offset;    /* Current position in BUFFER.  */
} mbbuf_t;

/* Initialize MBBUF with an allocated BUFFER of SIZE bytes and a file stream
   FP open for reading.  SIZE must be greater than or equal to MCEL_LEN_MAX.
 */
MBBUF_INLINE void
mbbuf_init (mbbuf_t *mbbuf, char *buffer, idx_t size, FILE *fp)
{
  if (size < MCEL_LEN_MAX)
    unreachable ();
  mbbuf->buffer = buffer;
  mbbuf->fp = fp;
  mbbuf->size = size;
  mbbuf->length = 0;
  mbbuf->offset = 0;
}

/* Get the next character in the buffer, filling it from FP if necessary.
   If an invalid multi-byte character is seen, we assume the program wants to
   fall back to the read byte.  */
MBBUF_INLINE mcel_t
mbbuf_get_char (mbbuf_t *mbbuf)
{
  idx_t available = mbbuf->length - mbbuf->offset;
  /* Check if we need to fill the input buffer.  */
  if (available < MCEL_LEN_MAX && ! feof (mbbuf->fp))
    {
      idx_t start;
      if (!(0 < available))
        start = 0;
      else
        {
          memmove (mbbuf->buffer, mbbuf->buffer + mbbuf->offset, available);
          start = available;
        }
      mbbuf->length = fread (mbbuf->buffer + start, 1, mbbuf->size - start,
                             mbbuf->fp) + start;
      mbbuf->offset = 0;
      available = mbbuf->length - mbbuf->offset;
    }
  if (available <= 0)
    return (mcel_t) { .ch = MBBUF_EOF };
  mcel_t g = mcel_scan (mbbuf->buffer + mbbuf->offset,
                        mbbuf->buffer + mbbuf->length);
  if (! g.err)
    mbbuf->offset += g.len;
  else
    {
      /* Assume the program will emit the byte, but keep the error flag.  */
      g.ch = mbbuf->buffer[mbbuf->offset++];
    }
  return g;
}

/* Returns a pointer to the first byte in the previously read character from
   mbbuf_get_char.  */
MBBUF_INLINE char *
mbbuf_char_offset (mbbuf_t *mbbuf, mcel_t g)
{
  if (mbbuf->offset < g.len)
    unreachable ();
  return mbbuf->buffer + (mbbuf->offset - g.len);
}

_GL_INLINE_HEADER_END

#endif
