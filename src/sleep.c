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
#include <assert.h>
#include <sys/types.h>
#include <time.h>
#include <getopt.h>

#include <math.h>
#if HAVE_FLOAT_H
# include <float.h>
#else
# define DBL_MAX 1.7976931348623159e+308
#endif

#ifndef TIME_T_MAX
# define TIME_T_MAX TYPE_MAXIMUM (time_t)
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

int
main (int argc, char **argv)
{
  int i;
  double seconds = 0.0;
  int c;
  int fail = 0;
  int interrupted;
  struct timespec ts;

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
      if (xstrtod (argv[i], &p, &s)
	  /* No negative intervals.  */
	  || s < 0
	  /* No extra chars after the number and an optional s,m,h,d char. */
	  || (*p && *(p+1))
	  /* Update S based on suffix char.  */
	  || apply_suffix (&s, *p)
	  /* Make sure the sum fits in a time_t.  */
	  || (seconds += s) > TIME_T_MAX
	  )
	{
	  error (0, 0, _("invalid time interval `%s'"), argv[i]);
	  fail = 1;
	}
    }

  if (fail)
    usage (1);

  ts.tv_sec = seconds;
  ts.tv_nsec = (int) ((seconds - ts.tv_sec) * 1000000000 + .5);

  while (1)
    {
      struct timespec remaining;
      interrupted = nanosleep (&ts, &remaining);
      assert (!interrupted || errno == EINTR);
      if (!interrupted)
	break;
      ts = remaining;
    }

  exit (0);
}
