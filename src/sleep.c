/* sleep - delay for a specified amount of time.
   Copyright (C) 84, 1991-1997, 1999-2005 Free Software Foundation, Inc.

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

#include <config.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"
#include "c-strtod.h"
#include "error.h"
#include "long-options.h"
#include "quote.h"
#include "xnanosleep.h"
#include "xstrtod.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "sleep"

#define AUTHORS "Jim Meyering", "Paul Eggert"

/* The name by which this program was run. */
char *program_name;

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
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
   and return false.  Otherwise return true.  */

static bool
apply_suffix (double *x, char suffix_char)
{
  int multiplier;

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
      return false;
    }

  *x *= multiplier;

  return true;
}

int
main (int argc, char **argv)
{
  int i;
  double seconds = 0.0;
  bool ok = true;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      usage, AUTHORS, (char const *) NULL);
  if (getopt_long (argc, argv, "", NULL, NULL) != -1)
    usage (EXIT_FAILURE);

  if (argc == 1)
    {
      error (0, 0, _("missing operand"));
      usage (EXIT_FAILURE);
    }

  for (i = optind; i < argc; i++)
    {
      double s;
      const char *p;
      if (! xstrtod (argv[i], &p, &s, c_strtod)
	  /* Nonnegative interval.  */
	  || ! (0 <= s)
	  /* No extra chars after the number and an optional s,m,h,d char.  */
	  || (*p && *(p+1))
	  /* Check any suffix char and update S based on the suffix.  */
	  || ! apply_suffix (&s, *p))
	{
	  error (0, 0, _("invalid time interval %s"), quote (argv[i]));
	  ok = false;
	}

      seconds += s;
    }

  if (!ok)
    usage (EXIT_FAILURE);

  if (xnanosleep (seconds))
    error (EXIT_FAILURE, errno, _("cannot read realtime clock"));

  exit (EXIT_SUCCESS);
}
