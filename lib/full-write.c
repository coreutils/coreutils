/* An interface to write() that writes all it is asked to write.

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
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "full-write.h"

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include "safe-write.h"

/* Write COUNT bytes at BUF to descriptor FD, retrying if interrupted
   or if partial writes occur.  Return the number of bytes successfully
   written, setting errno if that is less than COUNT.  */
size_t
full_write (int fd, const void *buf, size_t count)
{
  size_t total_written = 0;
  const char *ptr = buf;

  while (count > 0)
    {
      size_t written = safe_write (fd, ptr, count);
      if (written == (size_t)-1)
	break;
      if (written == 0)
	{
	  /* Some buggy drivers return 0 when you fall off a device's
	     end.  (Example: Linux 1.2.13 on /dev/fd0.)  */
	  errno = ENOSPC;
	  break;
	}
      total_written += written;
      ptr += written;
      count -= written;
    }

  return total_written;
}
