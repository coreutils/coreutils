/* sleep - delay for a specified amount of time.
   Copyright (C) 84, 1991-1997, 1999-2003 Free Software Foundation, Inc.

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
#include <getopt.h>

#include "system.h"
#include "closeout.h"
#include "error.h"
#include "long-options.h"
#include "xnanosleep.h"
#include "xstrtod.h"

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

int
main (int argc, char **argv)
{
  int i;
  double seconds = 0.0;
  int c;
  int fail = 0;

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

  if (xnanosleep (seconds))
    error (EXIT_FAILURE, errno, _("cannot read realtime clock"));

  exit (EXIT_SUCCESS);
}
