/* xnanosleep.c -- a more convenient interface to nanosleep
   Copyright (C) 2002, 2003 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>

#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif

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

#include "timespec.h"
#include "xalloc.h"
#include "xnanosleep.h"

/* Subtract the `struct timespec' values X and Y by computing X - Y.
   If the difference is negative or zero, return 0.
   Otherwise, return 1 and store the difference in DIFF.
   X and Y must have valid ts_nsec values, in the range 0 to 999999999.
   If the difference would overflow, store the maximum possible difference.  */

static int
timespec_subtract (struct timespec *diff,
		   struct timespec const *x, struct timespec const *y)
{
  time_t sec = x->tv_sec - y->tv_sec;
  long int nsec = x->tv_nsec - y->tv_nsec;

  if (x->tv_sec < y->tv_sec)
    return 0;

  if (sec < 0)
    {
      /* The difference has overflowed.  */
      sec = TIME_T_MAX;
      nsec = 999999999;
    }
  else if (sec == 0 && nsec <= 0)
    return 0;

  if (nsec < 0)
    {
      sec--;
      nsec += 1000000000;
    }

  diff->tv_sec = sec;
  diff->tv_nsec = nsec;
  return 1;
}

/* Sleep until the time (call it WAKE_UP_TIME) specified as
   SECONDS seconds after the time this function is called.
   SECONDS must be non-negative.  If SECONDS is so large that
   it is not representable as a `struct timespec', then use
   the maximum value for that interval.  Return -1 on failure
   (setting errno), 0 on success.  */

int
xnanosleep (double seconds)
{
  int overflow;
  double ns;
  struct timespec ts_start;
  struct timespec ts_sleep;
  struct timespec ts_stop;

  assert (0 <= seconds);

  if (gettime (&ts_start) != 0)
    return -1;

  /* Separate whole seconds from nanoseconds.
     Be careful to detect any overflow.  */
  ts_sleep.tv_sec = seconds;
  ns = 1e9 * (seconds - ts_sleep.tv_sec);
  overflow = ! (ts_sleep.tv_sec <= seconds && 0 <= ns && ns <= 1e9);
  ts_sleep.tv_nsec = ns;

  /* Round up to the next whole number, if necessary, so that we
     always sleep for at least the requested amount of time.  Assuming
     the default rounding mode, we don't have to worry about the
     rounding error when computing 'ns' above, since the error won't
     cause 'ns' to drop below an integer boundary.  */
  ts_sleep.tv_nsec += (ts_sleep.tv_nsec < ns);

  /* Normalize the interval length.  nanosleep requires this.  */
  if (1000000000 <= ts_sleep.tv_nsec)
    {
      time_t t = ts_sleep.tv_sec + 1;

      /* Detect integer overflow.  */
      overflow |= (t < ts_sleep.tv_sec);

      ts_sleep.tv_sec = t;
      ts_sleep.tv_nsec -= 1000000000;
    }

  /* Compute the time until which we should sleep.  */
  ts_stop.tv_sec = ts_start.tv_sec + ts_sleep.tv_sec;
  ts_stop.tv_nsec = ts_start.tv_nsec + ts_sleep.tv_nsec;
  if (1000000000 <= ts_stop.tv_nsec)
    {
      ++ts_stop.tv_sec;
      ts_stop.tv_nsec -= 1000000000;
    }

  /* Detect integer overflow.  */
  overflow |= (ts_stop.tv_sec < ts_start.tv_sec
	       || (ts_stop.tv_sec == ts_start.tv_sec
		   && ts_stop.tv_nsec < ts_start.tv_nsec));

  if (overflow)
    {
      /* Fix ts_sleep and ts_stop, which may be garbage due to overflow.  */
      ts_sleep.tv_sec = ts_stop.tv_sec = TIME_T_MAX;
      ts_sleep.tv_nsec = ts_stop.tv_nsec = 999999999;
    }

  while (nanosleep (&ts_sleep, NULL) != 0)
    {
      if (errno != EINTR || gettime (&ts_start) != 0)
	return -1;

      /* POSIX.1-2001 requires that when a process is suspended, then
	 resumed, nanosleep (A, B) returns -1, sets errno to EINTR,
	 and sets *B to the time remaining at the point of resumption.
	 However, some versions of the Linux kernel incorrectly return
	 the time remaining at the point of suspension.  Work around
	 this bug by computing the remaining time here, rather than by
	 relying on nanosleep's computation.  */

      if (! timespec_subtract (&ts_sleep, &ts_stop, &ts_start))
	break;
    }

  return 0;
}
