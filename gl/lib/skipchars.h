/* Skipping sequences of characters satisfying a predicate

   Copyright 2023-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include "mcel.h"

_GL_INLINE_HEADER_BEGIN
#ifndef SKIPCHARS_INLINE
# define SKIPCHARS_INLINE _GL_INLINE
#endif

/* Return the address just past the leading sequence of possibly
   multi-byte characters or encoding errors G in STR that satisfy
   PREDICATE (G) if OK is true, or that do not satisfy the predicate
   call if OK is false.  */

SKIPCHARS_INLINE char *
skip_str_matching (char const *str, bool (*predicate) (mcel_t), bool ok)
{
  char const *s = str;
  for (mcel_t g; *s && predicate (g = mcel_scanz (s)) == ok;
       s += g.len)
    continue;
  return (char *) s;
}

/* Return the address just past the leading sequence of possibly
   multi-byte characters or encoding errors G in BUF (which ends at LIM)
   that satisfy PREDICATE (G) if OK is true, or that do not satisfy
   the predicate call if OK is false.  */

SKIPCHARS_INLINE char *
skip_buf_matching (char const *buf, char const *lim,
                   bool (*predicate) (mcel_t), bool ok)
{
  char const *s = buf;
  for (mcel_t g; s < lim && predicate (g = mcel_scan (s, lim)) == ok;
       s += g.len)
    continue;
  return (char *) s;
}

_GL_INLINE_HEADER_END
