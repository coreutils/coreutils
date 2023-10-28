/* Character type definitions for coreutils

   Copyright 2023 Free Software Foundation, Inc.

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

#include <ctype.h>

#ifndef _GL_INLINE_HEADER_BEGIN
# error "Please include config.h first."
#endif
_GL_INLINE_HEADER_BEGIN
#ifndef CU_CTYPE_INLINE
# define CU_CTYPE_INLINE _GL_INLINE
#endif

/* '\n' is considered a field separator with  --zero-terminated.  */
CU_CTYPE_INLINE bool
field_sep (unsigned char ch)
{
  return isblank (ch) || ch == '\n';
}

_GL_INLINE_HEADER_END
