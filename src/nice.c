/* nice -- run a program with modified scheduling priority
   Copyright (C) 1990-2002 Free Software Foundation, Inc.

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
#ifndef NICE_PRIORITY
# include <sys/time.h>
# include <sys/resource.h>
#endif

#include "system.h"
#include "closeout.h"
#include "error.h"
#include "long-options.h"
#include "posixver.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "nice"

#define AUTHORS "David MacKenzie"

#ifdef NICE_PRIORITY
# define GET_PRIORITY() nice (0)
#else
# define GET_PRIORITY() getpriority (PRIO_PROCESS, 0)
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
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION] [COMMAND [ARG]...]\n"), program_name);
      fputs (_("\
Run COMMAND with an adjusted scheduling priority.\n\
With no COMMAND, print the current scheduling priority.  ADJUST is 10\n\
by default.  Range goes from -20 (highest priority) to 19 (lowest).\n\
\n\
  -n, --adjustment=ADJUST   increment priority by ADJUST first\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int current_priority;
  long int adjustment = 0;
  int minusflag = 0;
  int adjustment_given = 0;
  int i;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      AUTHORS, usage);

  for (i = 1; i < argc; /* empty */)
    {
      char *s = argv[i];

      if (s[0] == '-' && s[1] == '-' && ISDIGIT (s[2])
	  && posix2_version () < 200112)
	{
	  if (xstrtol (&s[2], NULL, 10, &adjustment, "") != LONGINT_OK)
	    error (EXIT_FAILURE, 0, _("invalid option `%s'"), s);

	  minusflag = 1;
	  adjustment_given = 1;
	  ++i;
	}
      else if (s[0] == '-'
	       && (ISDIGIT (s[1]) || (s[1] == '+' && ISDIGIT (s[2])))
	       && posix2_version () < 200112)
	{
	  if (s[1] == '+')
	    ++s;
	  if (xstrtol (&s[1], NULL, 10, &adjustment, "") != LONGINT_OK)
	    error (EXIT_FAILURE, 0, _("invalid option `%s'"), s);

	  minusflag = 0;
	  adjustment_given = 1;
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
		  usage (EXIT_FAILURE);

		case 'n':
		  if (xstrtol (optarg, NULL, 10, &adjustment, "")
		      != LONGINT_OK)
		    error (EXIT_FAILURE, 0, _("invalid priority `%s'"), optarg);

		  minusflag = 0;
		  adjustment_given = 1;
		  break;
		}
	    }

	  i += optind - 1;

	  if (optc == EOF)
	    break;
	}
    }

  if (minusflag)
    adjustment = -adjustment;
  if (!adjustment_given)
    adjustment = 10;

  if (i == argc)
    {
      if (adjustment_given)
	{
	  error (0, 0, _("a command must be given with an adjustment"));
	  usage (EXIT_FAILURE);
	}
      /* No command given; print the priority. */
      errno = 0;
      current_priority = GET_PRIORITY ();
      if (current_priority == -1 && errno != 0)
	error (EXIT_FAILURE, errno, _("cannot get priority"));
      printf ("%d\n", current_priority);
      exit (EXIT_SUCCESS);
    }

#ifndef NICE_PRIORITY
  errno = 0;
  current_priority = GET_PRIORITY ();
  if (current_priority == -1 && errno != 0)
    error (EXIT_FAILURE, errno, _("cannot get priority"));
  if (setpriority (PRIO_PROCESS, 0, current_priority + adjustment))
#else
  if (nice (adjustment) == -1)
#endif
    error (EXIT_FAILURE, errno, _("cannot set priority"));

  execvp (argv[i], &argv[i]);

  {
    int exit_status = (errno == ENOENT ? 127 : 126);
    error (0, errno, "%s", argv[i]);
    exit (exit_status);
  }
}
