/* sleep - delay for a specified amount of time.
   Copyright (C) 84, 1991-1997, 1999 Free Software Foundation, Inc.

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
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <getopt.h>

#include <math.h>
#if HAVE_FLOAT_H
# include <float.h>
#else
# define DBL_MAX 1.7976931348623159e+308
# define DBL_MIN 2.2250738585072010e-308
#endif

#include "system.h"
#include "error.h"
#include "long-options.h"
#include "xstrtod.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "sleep"

#define AUTHORS "Jim Meyering"

/* The name by which this program was run. */
char *program_name;

/* This is set once we've received the SIGCONT signal.  */
static int suspended;

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
Pause for NUMBER seconds.\n\
SUFFIX may be s for seconds (the default), m for minutes,\n\
h for hours or d for days.\n\
\n\
  --help      display this help and exit\n\
  --version   output version information and exit\n\
"),
	      program_name, program_name);
      puts (_("\nReport bugs to <bug-sh-utils@gnu.org>."));
    }
  exit (status);
}

/* Handle SIGCONT. */

static void
sighandler (int sig)
{
#ifdef SA_INTERRUPT
  struct sigaction sigact;

  sigact.sa_handler = SIG_DFL;
  sigemptyset (&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction (sig, &sigact, NULL);
#else
  signal (sig, SIG_DFL);
#endif
  printf ("in handler\n");

  suspended = 1;
  kill (getpid (), sig);
}

/* FIXME: describe */

static int
apply_suffix (double *s, char suffix_char)
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

  if (multiplier == 0 || *s > DBL_MAX / multiplier)
    return 1;

  *s *= multiplier;

  return 0;
}

/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.
   From the GNU libc manual.  */

static int
timeval_subtract (struct timeval *result,
		  const struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating Y. */
  if (x->tv_usec < y->tv_usec)
    {
      int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
      y->tv_usec -= 1000000 * nsec;
      y->tv_sec += nsec;
    }

  if (x->tv_usec - y->tv_usec > 1000000)
    {
      int nsec = (y->tv_usec - x->tv_usec) / 1000000;
      y->tv_usec += 1000000 * nsec;
      y->tv_sec -= nsec;
    }

  /* Compute the time remaining to wait.
     `tv_usec' is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

int
main (int argc, char **argv)
{
  int i;
  double seconds = 0.0;
  int c;
  int fail = 0;
  struct timeval tv_start;
  struct timeval tv_done;
  int i_sec;
  int i_usec;
#ifdef SA_INTERRUPT
  struct sigaction oldact, newact;
#endif

  /* FIXME */
  gettimeofday (&tv_start, NULL);

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      AUTHORS, usage);

  while ((c = getopt_long (argc, argv, "", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	default:
	  usage (1);
	}
    }

  if (argc == 1)
    {
      error (0, 0, _("too few arguments"));
      usage (1);
    }

  for (i = optind; i < argc; i++)
    {
      double s;
      const char *p;
      if (xstrtod (argv[i], &p, &s) || s < 0 || apply_suffix (&s, *p))
	{
	  error (0, 0, _("invalid time interval `%s'"), argv[i]);
	  fail = 1;
	  continue;
	}
      seconds += s;
    }


  if (fail)
    usage (1);

#ifdef SA_INTERRUPT
  newact.sa_handler = sighandler;
  sigemptyset (&newact.sa_mask);
  newact.sa_flags = 0;

  sigaction (SIGCONT, NULL, &oldact);
  if (oldact.sa_handler != SIG_IGN)
    sigaction (SIGCONT, &newact, NULL);
#else				/* !SA_INTERRUPT */
  if (signal (SIGCONT, SIG_IGN) != SIG_IGN)
    signal (SIGCONT, sighandler);
#endif				/* !SA_INTERRUPT */

  i_sec = floor (seconds);
  sleep (i_sec);
  i_usec = (int) ((seconds - i_sec) * 1000000);
  usleep (i_usec);

  if (!suspended)
    exit (0);

  tv_done.tv_sec = tv_start.tv_sec + i_sec;
  tv_done.tv_usec = tv_start.tv_usec + i_usec;

  while (1)
    {
      struct timeval diff;
      struct timeval tv_now;
      int negative;
      gettimeofday (&tv_now, NULL);
      negative = timeval_subtract (&diff, &tv_done, &tv_now);
      if (negative)
	break;
      sleep (diff.tv_sec);
      usleep (diff.tv_usec);
    }

  exit (0);
}
