/* exclude.c -- exclude file names

   Copyright 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Jim Meyering <jim@meyering.net>  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "xalloc.h"
#include "xreadlink.h"

/* Call readlink to get the symbolic link value of FILENAME.
   Return a pointer to that NUL-terminated string in malloc'd storage.
   If readlink fails, return NULL (use errno to diagnose).
   If realloc fails, or if the link value is longer than SIZE_MAX :-),
   give a diagnostic and exit.  */

char *
xreadlink (char const *filename, size_t *link_length_arg)
{
  size_t buf_size = 128;  /* must be a power of 2 */
  char *buffer = NULL;

  while (1)
    {
      int link_length;
      buffer = (char *) xrealloc (buffer, buf_size);
      link_length = readlink (filename, buffer, buf_size);
      if (link_length < 0)
	{
	  free (buffer);
	  return NULL;
	}
      if (link_length < buf_size)
	{
	  *link_length_arg = link_length;
	  buffer[link_length] = 0;
	  return buffer;
	}
      buf_size *= 2;
      if (buf_size == 0)
	xalloc_die ();
    }
}
