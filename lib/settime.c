/* settime -- set the system clock
   Copyright (C) 2002, 2004, 2005 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Paul Eggert.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "timespec.h"

#include <unistd.h>

#include <errno.h>

/* Some systems don't have ENOSYS.  */
#ifndef ENOSYS
# ifdef ENOTSUP
#  define ENOSYS ENOTSUP
# else
/* Some systems don't have ENOTSUP either.  */
#  define ENOSYS EINVAL
# endif
#endif

/* Set the system time.  */

int
settime (struct timespec const *ts)
{
#if defined CLOCK_REALTIME && HAVE_CLOCK_SETTIME
  {
    int r = clock_settime (CLOCK_REALTIME, ts);
    if (r == 0 || errno == EPERM)
      return r;
  }
#endif

#if HAVE_SETTIMEOFDAY
  {
    struct timeval tv;

    tv.tv_sec = ts->tv_sec;
    tv.tv_usec = ts->tv_nsec / 1000;
    return settimeofday (&tv, 0);
  }
#elif HAVE_STIME
  /* This fails to compile on OSF1 V5.1, due to stime requiring
     a `long int*' and tv_sec is `int'.  But that system does provide
     settimeofday.  */
  return stime (&ts->tv_sec);
#else
  errno = ENOSYS;
  return -1;
#endif
}
