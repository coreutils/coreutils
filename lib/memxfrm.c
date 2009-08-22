/* Locale-specific memory transformation

   Copyright (C) 2006, 2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Paul Eggert <eggert@cs.ucla.edu>.  */

#include <config.h>

#include "memxfrm.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* Store into DEST (of size DESTSIZE) the text in SRC (of size SRCSIZE)
   transformed so that the result of memcmp on two transformed texts
   (with ties going to the longer text) is the same as the result of
   memcoll on the two texts before their transformation.  Perhaps
   temporarily modify the byte after SRC, but restore its original
   contents before returning.

   Return the size of the resulting text, or an indeterminate value if
   there is an error.  Set errno to an error number if there is an
   error, and to zero otherwise.  DEST contains an indeterminate value
   if there is an error or if the resulting size is greater than
   DESTSIZE.  */

size_t
memxfrm (char *restrict dest, size_t destsize,
         char *restrict src, size_t srcsize)
{
#if HAVE_STRXFRM

  size_t di = 0;
  size_t si = 0;
  size_t result = 0;

  char ch = src[srcsize];
  src[srcsize] = '\0';

  while (si < srcsize)
    {
      size_t slen = strlen (src + si);

      size_t result0 = result;
      errno = 0;
      result += strxfrm (dest + di, src + si, destsize - di) + 1;
      if (errno != 0)
        break;
      if (result <= result0)
        {
          errno = ERANGE;
          break;
        }

      if (result == destsize + 1 && si + slen == srcsize)
        {
          /* The destination is exactly the right size, but strxfrm wants
             room for a trailing null.  Work around the problem with a
             temporary buffer.  */
          size_t bufsize = destsize - di + 1;
          char stackbuf[4000];
          char *buf = stackbuf;
          if (sizeof stackbuf < bufsize)
            {
              buf = malloc (bufsize);
              if (! buf)
                break;
            }
          strxfrm (buf, src + si, bufsize);
          memcpy (dest + di, buf, destsize - di);
          if (sizeof stackbuf < bufsize)
            free (buf);
          errno = 0;
        }

      di = (result < destsize ? result : destsize);
      si += slen + 1;
    }

  src[srcsize] = ch;
  return result - (si != srcsize);

#else

  if (srcsize < destsize)
    memcpy (dest, src, srcsize);
  errno = 0;
  return srcsize;

#endif
}
