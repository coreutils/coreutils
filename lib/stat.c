/* Work around the bug in some systems whereby stat succeeds when
   given the zero-length file name argument.  The stat from SunOS4.1.4
   has this bug.
   Copyright (C) 1997, 1998 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#include <config.h>

/* Disable the definition of stat to rpl_stat (from config.h) in this
   file.  Otherwise, we'd get conflicting prototypes for rpl_stat on
   most systems.  */
#undef stat

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifndef errno
extern int errno;
#endif

/* This is a wrapper for stat(2).
   If FILE is the empty string, fail with errno == ENOENT.
   Otherwise, return the result of calling the real stat.

   This works around the bug in some systems whereby stat succeeds when
   given the zero-length file name argument.  The stat from SunOS4.1.4
   has this bug.  */

int
rpl_stat (file, sbuf)
     const char *file;
     struct stat *sbuf;
{
  if (file && *file == 0)
    {
      errno = ENOENT;
      return -1;
    }

  return stat (file, sbuf);
}
