/* gettime -- get the system clock
   Copyright (C) 2002 Free Software Foundation, Inc.

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

/* Written by Paul Eggert.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "timespec.h"

/* Get the system time.  */

int
gettime (struct timespec *ts)
{
#if defined CLOCK_REALTIME && HAVE_CLOCK_GETTIME
  if (clock_gettime (CLOCK_REALTIME, ts) == 0)
    return 0;
#endif

  {
    struct timeval tv;
    int r = gettimeofday (&tv, 0);
    if (r == 0)
      {
	ts->tv_sec = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000;
      }
    return r;
  }
}
