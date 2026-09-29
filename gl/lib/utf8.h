/* UTF-8 utilities.
   Copyright (C) 2026 Free Software Foundation, Inc.

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

#ifndef _GL_UTF8_H
#define _GL_UTF8_H 1

#ifndef _GL_INLINE_HEADER_BEGIN
# error "Please include config.h first."
#endif

#include <uchar.h>
#include <wchar.h>
#include <stdint.h>

#include "idx.h"
#include "unistr.h"

/* Return true if the current charset is UTF-8.  */
static inline bool
is_utf8_charset (void)
{
  static int is_utf8 = -1;
  if (is_utf8 == -1)
    {
      char32_t w;
      mbstate_t mbs; mbszero (&mbs);
      is_utf8 = mbrtoc32 (&w, "\xe2\x9f\xb8", 3, &mbs) == 3 && w == 0x27F8;
    }
  return is_utf8;
}

_GL_INLINE_HEADER_BEGIN
#ifndef UTF8_INLINE
# define UTF8_INLINE _GL_INLINE
#endif

/* Count characters in the valid UTF-8 prefix of BUF's *NBYTES bytes.
   Set *NBYTES to the length of that prefix, stopping before an invalid
   or incomplete sequence.  ASCII bytes, including NUL, count as characters.  */

UTF8_INLINE idx_t
u8_count (char const *buf, idx_t *nbytes)
{
  uint8_t const *p = (uint8_t const *) buf;
  idx_t n = *nbytes;

  /* Detect non-ASCII.  */
  unsigned char bits = n ? p[0] : 0;
  if (!(bits & 0x80))
    for (idx_t i = 0; i < n; i++)
      bits |= p[i];
  if (!(bits & 0x80))
    return n;

  uint8_t const *invalid = u8_check (p, n);
  if (invalid)
    *nbytes = n = invalid - p;

  /* Count each non-continuation byte, i.e., each UTF-8 character.  */
  idx_t count = 0;
  for (idx_t i = 0; i < n; i++)
    count += (p[i] & 0xc0) != 0x80;
  return count;
}

_GL_INLINE_HEADER_END

#endif
