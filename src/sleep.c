/* sleep - delay for a specified amount of time.
   Copyright (C) 84, 1991-1997, 1999-2002 Free Software Foundation, Inc.

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

#include <config.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <time.h>
#include <getopt.h>

#define USE_CLOCK_GETTIME (defined CLOCK_REALTIME && HAVE_CLOCK_GETTIME)
#if ! USE_CLOCK_GETTIME
# include <sys/time.h>
#endif

#ifndef TIME_T_MAX
# define TIME_T_MAX TYPE_MAXIMUM (time_t)
#endif

#include "system.h"
#include "error.h"
#include "long-options.h"
#include "timespec.h"
#include "xstrtod.h"
#include "closeout.h"

#if HAVE_FENV_H
# include <fenv.h>
#endif

/* Tell the compiler that non-default rounding modes are used.  */
#if 199901 <= __STDC_VERSION__
 #pragma STDC FENV_ACCESS ON
#endif

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "sleep"

#define AUTHORS N_ ("Jim Meyering and Paul Eggert")

/* The name by which this program was run. */
char *program_name;

static struct option const long_options[] =
{
  {0, 0, 0, 0}
};

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s NUMBER[SUFFIX]...\n\
  or:  %s OPTION\n\
Pause for NUMBER seconds.  SUFFIX may be `s' for seconds (the default),\n\
`m' for minutes, `h' for hours or `d' for days.  Unlike most implementations\n\
that require NUMBER be an integer, here NUMBER may be an arbitrary floating\n\
point number.\n\
\n\
"),
	      program_name, program_name);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

/* Given a floating point value *X, and a suffix character, SUFFIX_CHAR,
   scale *X by the multiplier implied by SUFFIX_CHAR.  SUFFIX_CHAR may
   be the NUL byte or `s' to denote seconds, `m' for minutes, `h' for
   hours, or `d' for days.  If SUFFIX_CHAR is invalid, don't modify *X
   and return nonzero.  Otherwise return zero.  */

static int
apply_suffix (double *x, char suffix_char)
{
  unsigned int multiplier;

  switch (suffix_char)
    {
    case 0:
    case 's':
      multiplier = 1;
      break;
    case 'm':
      multiplier = 60;
      break;
    case 'h':
      multiplier = 60 * 60;
      break;
    case 'd':
      multiplier = 60 * 60 * 24;
      break;
    default:
      multiplier = 0;
    }

  if (multiplier == 0)
    return 1;

  *x *= multiplier;

  return 0;
}

/* Subtract the `struct timespec' values X and Y,
   storing the difference in DIFF.
   Return 1 if the difference is positive, otherwise 0.
   Derived from code in the GNU libc manual.  */

static int
timespec_subtract (struct timespec *diff,
		   const struct timespec *x, struct timespec *y)
{
  /* Perform the carry for the later subtraction by updating Y. */
  if (x->tv_nsec < y->tv_nsec)
    {
      int nsec = (y->tv_nsec - x->tv_nsec) / 1000000000 + 1;
      y->tv_nsec -= 1000000000 * nsec;
      y->tv_sec += nsec;
    }

  if (1000000000 < x->tv_nsec - y->tv_nsec)
    {
      int nsec = (y->tv_nsec - x->tv_nsec) / 1000000000;
      y->tv_nsec += 1000000000 * nsec;
      y->tv_sec -= nsec;
    }

  /* Compute the time remaining to wait.
     `tv_nsec' is certainly positive. */
  diff->tv_sec = x->tv_sec - y->tv_sec;
  diff->tv_nsec = x->tv_nsec - y->tv_nsec;

  /* Return 1 if result is positive. */
  return y->tv_sec < x->tv_sec;
}

static struct timespec *
clock_get_realtime (struct timespec *ts)
{
  int fail;
#if USE_CLOCK_GETTIME
  fail = clock_gettime (CLOCK_REALTIME, ts);
#else
  struct timeval tv;
  fail = gettimeofday (&tv, NULL);
  if (!fail)
    {
      ts->tv_sec = tv.tv_sec;
      ts->tv_nsec = 1000 * tv.tv_usec;
    }
#endif

  if (fail)
    error (EXIT_FAILURE, errno, _("cannot read realtime clock"));

  return ts;
}

int
main (int argc, char **argv)
{
  int i;
  double seconds = 0.0;
  double ns;
  int c;
  int fail = 0;
  int forever;
  struct timespec ts_start;
  struct timespec ts_stop;
  struct timespec ts_sleep;

  /* Record start time.  */
  clock_get_realtime (&ts_start);

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      AUTHORS, usage);

  while ((c = getopt_long (argc, argv, "", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (argc == 1)
    {
      error (0, 0, _("too few arguments"));
      usage (EXIT_FAILURE);
    }

#ifdef FE_UPWARD
  /* Always round up, since we must sleep for at least the specified
     interval.  */
  fesetround (FE_UPWARD);
#endif

  for (i = optind; i < argc; i++)
    {
      double s;
      const char *p;
      if (xstrtod (argv[i], &p, &s)
	  /* Nonnegative interval.  */
	  || ! (0 <= s)
	  /* No extra chars after the number and an optional s,m,h,d char.  */
	  || (*p && *(p+1))
	  /* Check any suffix char and update S based on the suffix.  */
	  || apply_suffix (&s, *p))
	{
	  error (0, 0, _("invalid time interval `%s'"), argv[i]);
	  fail = 1;
	}

      seconds += s;
    }

  if (fail)
    usage (EXIT_FAILURE);

  /* Separate whole seconds from nanoseconds.
     Be careful to detect any overflow.  */
  ts_sleep.tv_sec = seconds;
  ns = 1e9 * (seconds - ts_sleep.tv_sec);
  forever = ! (ts_sleep.tv_sec <= seconds && 0 <= ns && ns <= 1e9);
  ts_sleep.tv_nsec = ns;

  /* Round up to the next whole number, if necessary, so that we
     always sleep for at least the requested amount of time.  */
  ts_sleep.tv_nsec += (ts_sleep.tv_nsec < ns);

  /* Normalize the interval length.  nanosleep requires this.  */
  if (1000000000 <= ts_sleep.tv_nsec)
    {
      time_t t = ts_sleep.tv_sec + 1;

      /* Detect integer overflow.  */
      forever |= (t < ts_sleep.tv_sec);

      ts_sleep.tv_sec = t;
      ts_sleep.tv_nsec -= 1000000000;
    }

  ts_stop.tv_sec = ts_start.tv_sec + ts_sleep.tv_sec;
  ts_stop.tv_nsec = ts_start.tv_nsec + ts_sleep.tv_nsec;
  if (1000000000 <= ts_stop.tv_nsec)
    {
      ++ts_stop.tv_sec;
      ts_stop.tv_nsec -= 1000000000;
    }

  /* Detect integer overflow.  */
  forever |= (ts_stop.tv_sec < ts_start.tv_sec
	      || (ts_stop.tv_sec == ts_start.tv_sec
		  && ts_stop.tv_nsec < ts_start.tv_nsec));

  if (forever)
    {
      /* Fix ts_sleep and ts_stop, which may be garbage due to overflow.  */
      ts_sleep.tv_sec = ts_stop.tv_sec = TIME_T_MAX;
      ts_sleep.tv_nsec = ts_stop.tv_nsec = 999999999;
    }

  while (nanosleep (&ts_sleep, NULL) != 0
	 && timespec_subtract (&ts_sleep, &ts_stop,
			       clock_get_realtime (&ts_start)))
    continue;

  exit (EXIT_SUCCESS);
}
