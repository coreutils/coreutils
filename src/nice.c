/* nice -- run a program with modified scheduling priority
   Copyright (C) 1990-2004 Free Software Foundation, Inc.

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

/* David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>
#include <stdio.h>

#include <assert.h>

#include <getopt.h>
#include <sys/types.h>

#include "system.h"

#ifndef NICE_PRIORITY
/* Include this after "system.h" so we're sure to have definitions
   (from time.h or sys/time.h) required for e.g. the ru_utime member.  */
# include <sys/resource.h>
#endif

#include "error.h"
#include "long-options.h"
#include "posixver.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "nice"

#define AUTHORS "David MacKenzie"

#ifdef NICE_PRIORITY
# define GET_NICE_VALUE() nice (0)
#else
# define GET_NICE_VALUE() getpriority (PRIO_PROCESS, 0)
#endif

#ifndef NZERO
# define NZERO 20
#endif

/* The name this program was run with. */
char *program_name;

static struct option const longopts[] =
{
  {"adjustment", required_argument, NULL, 'n'},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION] [COMMAND [ARG]...]\n"), program_name);
      printf (_("\
Run COMMAND with an adjusted nice value, which affects the scheduling priority.\n\
With no COMMAND, print the current nice value.  Nice values range from\n\
%d (most favorable scheduling) to %d (least favorable).\n\
\n\
  -n, --adjustment=N   add integer N to the nice value (default 10)\n\
"),
	      - NZERO, NZERO - 1);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int current_nice_value;
  int adjustment = 10;
  char const *adjustment_given = NULL;
  bool ok;
  int i;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (EXIT_FAIL);
  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      usage, AUTHORS, (char const *) NULL);

  for (i = 1; i < argc; /* empty */)
    {
      char *s = argv[i];

      if (s[0] == '-' && ISDIGIT (s[1 + (s[1] == '-' || s[1] == '+')])
	  && posix2_version () < 200112)
	{
	  adjustment_given = s + 1;
	  ++i;
	}
      else
	{
	  int optc;
	  char **fake_argv = argv + i - 1;

	  /* Initialize getopt_long's internal state.  */
	  optind = 0;

	  if ((optc = getopt_long (argc - (i - 1), fake_argv, "+n:",
				   longopts, NULL)) != -1)
	    {
	      switch (optc)
		{
		case '?':
		  usage (EXIT_FAIL);

		case 'n':
		  adjustment_given = optarg;
		  break;
		}
	    }

	  i += optind - 1;

	  if (optc == EOF)
	    break;
	}
    }

  if (adjustment_given)
    {
      /* If the requested adjustment is outside the valid range,
	 silently bring it to just within range; this mimics what
	 "setpriority" and "nice" do.  */
      enum { MIN_ADJUSTMENT = 1 - 2 * NZERO, MAX_ADJUSTMENT = 2 * NZERO - 1 };
      long int tmp;
      if (LONGINT_OVERFLOW < xstrtol (adjustment_given, NULL, 10, &tmp, ""))
	error (EXIT_FAIL, 0, _("invalid adjustment `%s'"), adjustment_given);
      adjustment = MAX (MIN_ADJUSTMENT, MIN (tmp, MAX_ADJUSTMENT));
    }

  if (i == argc)
    {
      if (adjustment_given)
	{
	  error (0, 0, _("a command must be given with an adjustment"));
	  usage (EXIT_FAIL);
	}
      /* No command given; print the nice value.  */
      errno = 0;
      current_nice_value = GET_NICE_VALUE ();
      if (current_nice_value == -1 && errno != 0)
	error (EXIT_FAIL, errno, _("cannot get priority"));
      printf ("%d\n", current_nice_value);
      exit (EXIT_SUCCESS);
    }

#ifndef NICE_PRIORITY
  errno = 0;
  current_nice_value = GET_NICE_VALUE ();
  if (current_nice_value == -1 && errno != 0)
    error (EXIT_FAIL, errno, _("cannot get priority"));
  ok = (setpriority (PRIO_PROCESS, 0, current_nice_value + adjustment) == 0);
#else
  errno = 0;
  ok = (nice (adjustment) != -1 || errno == 0);
#endif
  if (!ok)
    error (errno == EPERM ? 0 : EXIT_FAIL, errno, _("cannot set priority"));

  execvp (argv[i], &argv[i]);

  {
    int exit_status = (errno == ENOENT ? EXIT_ENOENT : EXIT_CANNOT_INVOKE);
    error (0, errno, "%s", argv[i]);
    exit (exit_status);
  }
}
