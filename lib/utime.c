/* Copyright (C) 1998, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* derived from a function in touch.c */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#undef utime

#include <sys/types.h>

#ifdef HAVE_UTIME_H
# include <utime.h>
#endif

#if !HAVE_UTIMES_NULL
# include <sys/stat.h>
# include <fcntl.h>
#endif

#include <unistd.h>
#include <errno.h>

#include "full-write.h"
#include "safe-read.h"

/* Some systems (even some that do have <utime.h>) don't declare this
   structure anywhere.  */
#ifndef HAVE_STRUCT_UTIMBUF
struct utimbuf
{
  long actime;
  long modtime;
};
#endif

/* Emulate utime (file, NULL) for systems (like 4.3BSD) that do not
   interpret it to set the access and modification times of FILE to
   the current time.  Return 0 if successful, -1 if not. */

static int
utime_null (const char *file)
{
#if HAVE_UTIMES_NULL
  return utimes (file, 0);
#else
  int fd;
  char c;
  int status = 0;
  struct stat st;
  int saved_errno = 0;

  fd = open (file, O_RDWR);
  if (fd < 0
      || fstat (fd, &st) < 0
      || safe_read (fd, &c, sizeof c) == SAFE_READ_ERROR
      || lseek (fd, (off_t) 0, SEEK_SET) < 0
      || full_write (fd, &c, sizeof c) != sizeof c
      /* Maybe do this -- it's necessary on SunOS 4.1.3 with some combination
	 of patches, but that system doesn't use this code: it has utimes.
	 || fsync (fd) < 0
      */
      || (st.st_size == 0 && ftruncate (fd, st.st_size) < 0))
    {
      saved_errno = errno;
      status = -1;
    }

  if (0 <= fd)
    {
      if (close (fd) < 0)
	status = -1;

      /* If there was a prior failure, use the saved errno value.
	 But if the only failure was in the close, don't change errno.  */
      if (saved_errno)
	errno = saved_errno;
    }

  return status;
#endif
}

int
rpl_utime (const char *file, const struct utimbuf *times)
{
  if (times)
    return utime (file, times);

  return utime_null (file);
}
