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

#include "xmemxfrm.h"

#include <errno.h>
#include <stdlib.h>

#include "gettext.h"
#define _(msgid) gettext (msgid)

#include "error.h"
#include "exitfail.h"
#include "memxfrm.h"
#include "quotearg.h"

/* Store into DEST (of size DESTSIZE) the text in SRC (of size
   SRCSIZE) transformed so that the result of memcmp on two
   transformed texts (with ties going to the longer text) is the same
   as the result of memcoll on the two texts before their
   transformation.  Perhaps temporarily modify the byte after SRC, but
   restore its original contents before returning.

   Return the size of the resulting text.  DEST contains an
   indeterminate value if the resulting size is greater than DESTSIZE.
   Report an error and exit if there is an error.  */

size_t
xmemxfrm (char *restrict dest, size_t destsize,
          char *restrict src, size_t srcsize)
{
  size_t translated_size = memxfrm (dest, destsize, src, srcsize);

  if (errno)
    {
      error (0, errno, _("string transformation failed"));
      error (0, 0, _("set LC_ALL='C' to work around the problem"));
      error (exit_failure, 0,
             _("the untransformed string was %s"),
             quotearg_n_style_mem (0, locale_quoting_style, src, srcsize));
    }

  return translated_size;
}
