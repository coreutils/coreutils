/* xgethostname.c -- return current hostname with unlimited length
   Copyright (C) 1992 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Written by Jim Meyering, meyering@comco.com */

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

int gethostname ();
char *xmalloc ();
char *xrealloc ();

#ifndef INITIAL_HOSTNAME_LENGTH
#define INITIAL_HOSTNAME_LENGTH 33
#endif

char *
xgethostname ()
{
  char *hostname;
  size_t size;
  int err;

  size = INITIAL_HOSTNAME_LENGTH;
  while (1)
    {
      hostname = xmalloc (size);
      hostname[size - 1] = '\0';
      err = gethostname (hostname, size);
      if (err || hostname[size - 1] == '\0')
	break;
      size *= 2;
      hostname = xrealloc (hostname, size);
    }

  return hostname;
}
