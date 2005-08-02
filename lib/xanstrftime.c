/* xanstrftime.c -- format date/time into xmalloc'd storage
   Copyright (C) 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Jim Meyering. */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "xanstrftime.h"

#include "strftime.h"
#include "xalloc.h"

/* Return true if the nstrftime expansion of FORMAT (given DATE_TIME, etc.)
   is the empty string (this happens with %p in many locales).
   Otherwise, return false.  */
static bool
degenerate_format (char const *format, struct tm const *date_time,
		   int ut, int ns)
{
  size_t format_len = strlen (format);
  char *fake_fmt = xmalloc (format_len + 2);
  bool degenerate;
  char buf[2];

  /* Create a copy of FORMAT with a space prepended.  */
  fake_fmt[0] = ' ';
  memcpy (&fake_fmt[1], format, format_len + 1);

  degenerate = nstrftime (buf, sizeof buf, fake_fmt, date_time, ut, ns) == 1;
  free (fake_fmt);

  return degenerate;
}

/* Like nstrftime, but take no buffer or length parameters and
   instead return the formatted result in malloc'd storage.
   Call xalloc_die upon allocation failure.  */
char *
xanstrftime (char const *format, struct tm const *date_time, int ut, int ns)
{
  bool first = true;
  char *buf = NULL;
  size_t buf_length = 0;

  while (true)
    {
      buf = X2REALLOC (buf, &buf_length);
      if (nstrftime (buf, buf_length, format, date_time, ut, ns)
	  || (first && degenerate_format (format, date_time, ut, ns)))
	return buf;
      first = false;
    }
}
