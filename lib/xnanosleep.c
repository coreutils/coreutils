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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Mostly written (for sleep.c) by Paul Eggert.
   Factored out (creating this file) by Jim Meyering.  */

#if HAVE_CONFIG_H
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

#include "timespec.h"
#include "gethrxtime.h"
#include "xtime.h"

/* The extra casts work around common compiler bugs.  */
#define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))
/* The outer cast is needed to work around a bug in Cray C 5.0.3.0.
   It is necessary at least when t == time_t.  */
#define TYPE_MINIMUM(t) ((t) (TYPE_SIGNED (t) \
			      ? ~ (t) 0 << (sizeof (t) * CHAR_BIT - 1) : (t) 0))
#define TYPE_MAXIMUM(t) ((t) (~ (t) 0 - TYPE_MINIMUM (t)))

#ifndef TIME_T_MAX
# define TIME_T_MAX TYPE_MAXIMUM (time_t)
#endif

/* POSIX.1-2001 requires that when a process is suspended, then
   resumed, nanosleep (A, B) returns -1, sets errno to EINTR, and sets
   *B to the time remaining at the point of resumption.  However, some
   versions of the Linux kernel incorrectly return the time remaining
   at the point of suspension.  Work around this bug on GNU/Linux
   hosts by computing the remaining time here after nanosleep returns,
   rather than by relying on nanosleep's computation.  */
#ifdef __linux__
enum { NANOSLEEP_BUG_WORKAROUND = true };
#else
enum { NANOSLEEP_BUG_WORKAROUND = false };
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
  xtime_t stop = 0;

  assert (0 <= seconds);

  if (NANOSLEEP_BUG_WORKAROUND)
    {
      xtime_t now = gethrxtime ();
      double increment = XTIME_PRECISION * seconds;
      xtime_t incr = increment;
      stop = now + incr + (incr < increment);
      overflow = (stop < now);
    }

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

      if (nanosleep (&ts_sleep, NULL) == 0)
	break;
      if (errno != EINTR)
	return -1;

      if (NANOSLEEP_BUG_WORKAROUND)
	{
	  xtime_t now = gethrxtime ();
	  if (stop <= now)
	    break;
	  else
	    {
	      xtime_t remaining = stop - now;
	      ts_sleep.tv_sec = xtime_nonnegative_sec (remaining);
	      ts_sleep.tv_nsec = xtime_nonnegative_nsec (remaining);
	    }
	}
    }

  return 0;
}
