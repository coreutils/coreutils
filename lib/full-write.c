/* full-write.c -- an interface to write that retries after interrupts
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

   Copied largely from GNU C's cccp.c.
   */

#ifdef HAVE_CONFIG_H
#if defined (CONFIG_BROKETS)
/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because it found this file in $srcdir).  */
#include <config.h>
#else
#include "config.h"
#endif
#endif

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#ifndef STDC_HEADERS
extern int errno;
#endif

/* Write LEN bytes at PTR to descriptor DESC, retrying if interrupted.
   Return LEN upon success, write's (negative) error code otherwise.  */

int
full_write (desc, ptr, len)
     int desc;
     char *ptr;
     int len;
{
  int total_written;

  total_written = 0;
  while (len > 0)
    {
      int written = write (desc, ptr, len);
      if (written < 0)
	{
#ifdef EINTR
	  if (errno == EINTR)
	    continue;
#endif
	  return written;
	}
      total_written += written;
      ptr += written;
      len -= written;
    }
  return total_written;
}
