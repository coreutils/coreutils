/* safe-read.c -- an interface to read that retries after interrupts
   Copyright (C) 1993, 1994 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
   */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#ifndef STDC_HEADERS
extern int errno;
#endif

/* Read LEN bytes at PTR from descriptor DESC, for file FILENAME,
   retrying if necessary.  Return a negative value if an error occurs,
   otherwise return the actual number of bytes read,
   which must be LEN unless end-of-file was reached.  */

   The canonical source for this function is in the gcc distribution
   in file cccp.c.  So don't make any changes here.  */

static int
safe_read (desc, ptr, len)
     int desc;
     char *ptr;
     int len;
{
  int left = len;
  while (left > 0) {
    int nchars = read (desc, ptr, left);
    if (nchars < 0)
      {
#ifdef EINTR
	if (errno == EINTR)
	  continue;
#endif
	return nchars;
      }
    if (nchars == 0)
      break;
    ptr += nchars;
    left -= nchars;
  }
  return len - left;
}
