/* Buffering for multi-byte characters.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

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

MBBUF_INLINE idx_t
mbbuf_avail (mbbuf_t const *mbbuf)
{
  return mbbuf->length - mbbuf->offset;
}

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

/* Fill the input buffer with at least MIN_AVAILABLE bytes if possible.
   Return the number of bytes available from the current offset.  */
MBBUF_INLINE idx_t
mbbuf_fill (mbbuf_t *mbbuf, idx_t min_available)
{
  idx_t available = mbbuf_avail (mbbuf);

  if (mbbuf->size < min_available)
    min_available = mbbuf->size;

  if (available < min_available && ! feof (mbbuf->fp))
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
      available = mbbuf_avail (mbbuf);
    }

  return available;
}

/* Consume N bytes from the current buffer.  */
MBBUF_INLINE void
mbbuf_advance (mbbuf_t *mbbuf, idx_t n)
{
  if (mbbuf_avail (mbbuf) < n)
    unreachable ();
  mbbuf->offset += n;
}

/* Return the largest prefix of the current contents that is safe to process
   with byte searches, while leaving at least OVERLAP bytes unprocessed unless
   EOF has been seen.  The returned prefix never ends in the middle of a UTF-8
   sequence, but it may include invalid bytes.  */
MBBUF_INLINE idx_t
mbbuf_utf8_safe_prefix (mbbuf_t *mbbuf, idx_t overlap)
{
  idx_t available = mbbuf_fill (mbbuf, overlap + 4);
  if (available == 0)
    return 0;

  if (feof (mbbuf->fp))
    return available;

  if (available <= overlap)
    return 0;

  idx_t end = available - overlap;
  char const *buf = mbbuf->buffer + mbbuf->offset;
  idx_t start = end - 1;

  while (0 < start
         && ((unsigned char) buf[start] & 0xC0) == 0x80)
    start--;

  unsigned char lead = buf[start];
  idx_t len = (lead < 0x80 ? 1
               : (lead & 0xE0) == 0xC0 ? 2
               : (lead & 0xF0) == 0xE0 ? 3
               : (lead & 0xF8) == 0xF0 ? 4
               : 1);

  return start + len <= end ? end : start;
}

/* Get the next character in the buffer, filling it from FP if necessary.
   If an invalid multi-byte character is seen, we assume the program wants to
   fall back to the read byte.  */
MBBUF_INLINE mcel_t
mbbuf_get_char (mbbuf_t *mbbuf)
{
  idx_t available = mbbuf_fill (mbbuf, MCEL_LEN_MAX);
  if (available <= 0)
    return (mcel_t) { .ch = MBBUF_EOF };
  mcel_t g = mcel_scan (mbbuf->buffer + mbbuf->offset,
                        mbbuf->buffer + mbbuf->length);
  if (! g.err)
    mbbuf->offset += g.len;
  else
    {
      /* Assume the program will emit the byte, but keep the error flag.  */
      g.ch = (unsigned char) mbbuf->buffer[mbbuf->offset++];
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
