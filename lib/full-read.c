/* An interface to read() that reads all it is asked to read.

   Copyright (C) 1993, 1994, 1997, 1998, 1999, 2000, 2001, 2002 Free Software
   Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, read to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "full-read.h"

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include "safe-read.h"

/* Read COUNT bytes at BUF to descriptor FD, retrying if interrupted
   or if partial reads occur.  Return the number of bytes successfully
   read, setting errno if that is less than COUNT.  errno = 0 means EOF.  */
size_t
full_read (int fd, void *buf, size_t count)
{
  size_t total_read = 0;

  if (count > 0)
    {
      char *ptr = buf;

      do
	{
	  size_t nread = safe_read (fd, ptr, count);
	  if (nread == (size_t)-1)
	    break;
	  if (nread == 0)
	    {
	      /* EOF.  */
	      errno = 0;
	      break;
	    }
	  total_read += nread;
	  ptr += nread;
	  count -= nread;
	}
      while (count > 0);
    }

  return total_read;
}
