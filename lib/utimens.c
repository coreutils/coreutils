/* Copyright (C) 2003 Free Software Foundation, Inc.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Paul Eggert.  */

/* derived from a function in touch.c */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "utimens.h"

#if HAVE_UTIME_H
# include <utime.h>
#endif

/* Some systems (even some that do have <utime.h>) don't declare this
   structure anywhere.  */
#ifndef HAVE_STRUCT_UTIMBUF
struct utimbuf
{
  long actime;
  long modtime;
};
#endif

/* Set the access and modification time stamps of FILE to be
   TIMESPEC[0] and TIMESPEC[1], respectively.  */

int
utimens (char const *file, struct timespec const timespec[2])
{
  /* There's currently no interface to set file timestamps with
     nanosecond resolution, so do the best we can, discarding any
     fractional part of the timestamp.  */
#if HAVE_WORKING_UTIMES
  struct timeval timeval[2];
  timeval[0].tv_sec = timespec[0].tv_sec;
  timeval[0].tv_usec = timespec[0].tv_nsec / 1000;
  timeval[1].tv_sec = timespec[1].tv_sec;
  timeval[1].tv_usec = timespec[1].tv_nsec / 1000;
  return utimes (file, timeval);
#else
  struct utimbuf utimbuf;
  utimbuf.actime = timespec[0].tv_sec;
  utimbuf.modtime = timespec[1].tv_sec;
  return utime (file, &utimbuf);
#endif
}
