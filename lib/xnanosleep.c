/* xnanosleep.c -- a more convenient interface to nanosleep
   Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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

/* Mostly written (for sleep.c) by Paul Eggert.
   Factored out (creating this file) by Jim Meyering.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "xnanosleep.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>

#include "intprops.h"
#include "timespec.h"

#ifndef TIME_T_MAX
# define TIME_T_MAX TYPE_MAXIMUM (time_t)
#endif

/* Sleep until the time (call it WAKE_UP_TIME) specified as
   SECONDS seconds after the time this function is called.
   SECONDS must be non-negative.  If SECONDS is so large that
   it is not representable as a `struct timespec', then use
   the maximum value for that interval.  Return -1 on failure
   (setting errno), 0 on success.  */

int
xnanosleep (double seconds)
{
  enum { BILLION = 1000000000 };

  bool overflow = false;
  double ns;
  struct timespec ts_sleep;

  assert (0 <= seconds);

  /* Separate whole seconds from nanoseconds.
     Be careful to detect any overflow.  */
  ts_sleep.tv_sec = seconds;
  ns = BILLION * (seconds - ts_sleep.tv_sec);
  overflow |= ! (ts_sleep.tv_sec <= seconds && 0 <= ns && ns <= BILLION);
  ts_sleep.tv_nsec = ns;

  /* Round up to the next whole number, if necessary, so that we
     always sleep for at least the requested amount of time.  Assuming
     the default rounding mode, we don't have to worry about the
     rounding error when computing 'ns' above, since the error won't
     cause 'ns' to drop below an integer boundary.  */
  ts_sleep.tv_nsec += (ts_sleep.tv_nsec < ns);

  /* Normalize the interval length.  nanosleep requires this.  */
  if (BILLION <= ts_sleep.tv_nsec)
    {
      time_t t = ts_sleep.tv_sec + 1;

      /* Detect integer overflow.  */
      overflow |= (t < ts_sleep.tv_sec);

      ts_sleep.tv_sec = t;
      ts_sleep.tv_nsec -= BILLION;
    }

  for (;;)
    {
      if (overflow)
	{
	  ts_sleep.tv_sec = TIME_T_MAX;
	  ts_sleep.tv_nsec = BILLION - 1;
	}

      /* Linux-2.6.8.1's nanosleep returns -1, but doesn't set errno
	 when resumed after being suspended.  Earlier versions would
	 set errno to EINTR.  nanosleep from linux-2.6.10, as well as
	 implementations by (all?) other vendors, doesn't return -1
	 in that case;  either it continues sleeping (if time remains)
	 or it returns zero (if the wake-up time has passed).  */
      errno = 0;
      if (nanosleep (&ts_sleep, NULL) == 0)
	break;
      if (errno != EINTR && errno != 0)
	return -1;
    }

  return 0;
}
