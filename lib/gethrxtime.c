/* gethrxtime -- get high resolution real time

   Copyright (C) 2005 Free Software Foundation, Inc.

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

#include "gethrxtime.h"

#include <sys/types.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/* Get the time of a high-resolution clock, preferably one that is not
   subject to resetting or drifting.  */

xtime_t
gethrxtime (void)
{
#if HAVE_NANOUPTIME
  {
    struct timespec ts;
    nanouptime (&ts);
    return xtime_make (ts.tv_sec, ts.tv_nsec);
  }
#else

# if defined CLOCK_MONOTONIC && HAVE_CLOCK_GETTIME
  {
    struct timespec ts;
    if (clock_gettime (CLOCK_MONOTONIC, &ts) == 0)
      return xtime_make (ts.tv_sec, ts.tv_nsec);
  }
# endif

# if HAVE_MICROUPTIME
  {
    struct timeval tv;
    microuptime (&tv);
    return xtime_make (tv.tv_sec, 1000 * tv.tv_usec);
  }

  /* No monotonically increasing clocks are available; fall back on a
     clock that might jump backwards, since it's the best we can do.  */
# elif HAVE_GETTIMEOFDAY && XTIME_PRECISION != 1
  {
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return xtime_make (tv.tv_sec, 1000 * tv.tv_usec);
  }
# else
  return xtime_make (time (NULL), 0);
# endif
#endif
}
