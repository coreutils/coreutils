/* Provide a replacement for the POSIX nanosleep function.
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#include <sys/types.h>

#include <time.h>
/* FIXME: is including both like this kosher?  */
#include <sys/time.h>

static interrupted;

/* Sleep for USEC microseconds. */

static void
usleep (const struct timespec *ts_delay)
{
  struct timeval tv_delay;
  tv_delay.tv_sec = ts_delay->tv_sec;
  tv_delay.tv_usec = 1000 * ts_delay->tv_nsec;
  select (0, (void *) 0, (void *) 0, (void *) 0, tv_delay);
}

int
nanosleep (const struct timespec *requested_delay,
	   struct timespec *remaining_delay)
{
  interrupted = 0;

  /* set up sig handler -- but maybe only do this the first time?  */
  /* FIXME */

  usleep (requested_delay);

  if (interrupted)
    {
      /* Calculate time remaining.  */
      /* FIXME: the code in sleep doesn't use this, so there's no
	 rush to implement it.  */
    }

  /* FIXME: Restore sig handler?  */

  return interrupted;
}
