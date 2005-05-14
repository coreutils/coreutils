/* gettime -- get the system clock
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

/* Get the system time into *TS.  */

void
gettime (struct timespec *ts)
{
#if HAVE_NANOTIME
  nanotime (ts);
#else

# if defined CLOCK_REALTIME && HAVE_CLOCK_GETTIME
  if (clock_gettime (CLOCK_REALTIME, ts) == 0)
    return;
# endif

# if HAVE_GETTIMEOFDAY
  {
    struct timeval tv;
    gettimeofday (&tv, NULL);
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
  }
# else
  ts->tv_sec = time (NULL);
  ts->tv_nsec = 0;
# endif

#endif
}
