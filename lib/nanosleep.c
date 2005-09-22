/* Provide a replacement for the POSIX nanosleep function.
   Copyright (C) 1999, 2000, 2002, 2004, 2005 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Undefine nanosleep here so any prototype is not redefined to be a
   prototype for rpl_nanosleep. (they'd conflict e.g., on alpha-dec-osf3.2)  */
#undef nanosleep

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#include <errno.h>

#include <unistd.h>

#include "timespec.h"

/* Some systems (MSDOS) don't have SIGCONT.
   Using SIGTERM here turns the signal-handling code below
   into a no-op on such systems. */
#ifndef SIGCONT
# define SIGCONT SIGTERM
#endif

#if ! HAVE_SIGINTERRUPT
# define siginterrupt(sig, flag) /* empty */
#endif

static sig_atomic_t volatile suspended;

/* Handle SIGCONT. */

static void
sighandler (int sig)
{
  suspended = 1;
}

/* FIXME: comment */

static void
my_usleep (const struct timespec *ts_delay)
{
  struct timeval tv_delay;
  tv_delay.tv_sec = ts_delay->tv_sec;
  tv_delay.tv_usec = (ts_delay->tv_nsec + 999) / 1000;
  if (tv_delay.tv_usec == 1000000)
    {
      tv_delay.tv_sec++;
      tv_delay.tv_usec = 0;
    }
  select (0, NULL, NULL, NULL, &tv_delay);
}

/* FIXME: comment */

int
rpl_nanosleep (const struct timespec *requested_delay,
	       struct timespec *remaining_delay)
{
  static bool initialized;

  /* set up sig handler */
  if (! initialized)
    {
#ifdef SA_NOCLDSTOP
      struct sigaction oldact, newact;
      newact.sa_handler = sighandler;
      sigemptyset (&newact.sa_mask);
      newact.sa_flags = 0;

      sigaction (SIGCONT, NULL, &oldact);
      if (oldact.sa_handler != SIG_IGN)
	sigaction (SIGCONT, &newact, NULL);
#else
      if (signal (SIGCONT, SIG_IGN) != SIG_IGN)
	{
	  signal (SIGCONT, sighandler);
	  siginterrupt (SIGCONT, 1);
	}
#endif
      initialized = true;
    }

  suspended = 0;

  my_usleep (requested_delay);

  if (suspended)
    {
      /* Calculate time remaining.  */
      /* FIXME: the code in sleep doesn't use this, so there's no
	 rush to implement it.  */

      errno = EINTR;
    }

  /* FIXME: Restore sig handler?  */

  return suspended;
}
