/* An interface to read and write that retries after interrupts.
   Copyright (C) 1993, 1994, 1998, 2002-2003 Free Software Foundation, Inc.

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
#ifdef SAFE_WRITE
# include "safe-write.h"
#else
# include "safe-read.h"
#endif

/* Get ssize_t.  */
#include <sys/types.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#ifdef EINTR
# define IS_EINTR(x) ((x) == EINTR)
#else
# define IS_EINTR(x) 0
#endif

#include <limits.h>

#ifdef SAFE_WRITE
# define safe_rw safe_write
# define rw write
#else
# define safe_rw safe_read
# define rw read
# undef const
# define const /* empty */
#endif

/* Read(write) up to COUNT bytes at BUF from(to) descriptor FD, retrying if
   interrupted.  Return the actual number of bytes read(written), zero for EOF,
   or SAFE_READ_ERROR(SAFE_WRITE_ERROR) upon error.  */
size_t
safe_rw (int fd, void const *buf, size_t count)
{
  ssize_t result;

  /* POSIX limits COUNT to SSIZE_MAX, but we limit it further, requiring
     that COUNT <= INT_MAX, to avoid triggering a bug in Tru64 5.1.
     When decreasing COUNT, keep the file pointer block-aligned.
     Note that in any case, read(write) may succeed, yet read(write)
     fewer than COUNT bytes, so the caller must be prepared to handle
     partial results.  */
  if (count > INT_MAX)
    count = INT_MAX & ~8191;

  do
    {
      result = rw (fd, buf, count);
    }
  while (result < 0 && IS_EINTR (errno));

  return (size_t) result;
}
