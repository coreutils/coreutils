/* An interface to write() that retries after interrupts.
   Copyright (C) 1993, 1994, 1998, 2002 Free Software Foundation, Inc.

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
#include "safe-write.h"

/* Get ssize_t.  */
#include <sys/types.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include <limits.h>

/* We don't pass an nbytes count > SSIZE_MAX to write() - POSIX says the
   effect would be implementation-defined.  Also we don't pass an nbytes
   count > INT_MAX but <= SSIZE_MAX to write() - this triggers a bug in
   Tru64 5.1.  */
#define MAX_BYTES_TO_READ INT_MAX

/* Write up to COUNT bytes at BUF to descriptor FD, retrying if interrupted.
   Return the actual number of bytes written, zero for EOF, or (size_t) -1
   for an error.  */
size_t
safe_write (int fd, const void *buf, size_t count)
{
  size_t total_written = 0;

  if (count > 0)
    {
      const char *ptr = (const char *) buf;
      do
	{
	  size_t nbytes_to_write = count;
	  ssize_t result;

	  /* Limit the number of bytes to write in one round, to avoid running
	     into unspecified behaviour.  But keep the file pointer block
	     aligned when doing so.  */
	  if (nbytes_to_write > MAX_BYTES_TO_READ)
	    nbytes_to_write = MAX_BYTES_TO_READ & ~8191;

	  result = write (fd, ptr, nbytes_to_write);
	  if (result < 0)
	    {
#ifdef EINTR
	      if (errno == EINTR)
		continue;
#endif
	      return result;
	    }
	  total_written += result;
	  ptr += result;
	  count -= result;
	}
      while (count > 0);
    }

  return total_written;
}
