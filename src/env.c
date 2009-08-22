/* env - run a program in a modified environment
   Copyright (C) 1986, 1991-2005, 2007-2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Richard Mlynarik and David MacKenzie */

/* Options:
   -
   -i
   --ignore-environment
        Construct a new environment from scratch; normally the
        environment is inherited from the parent process, except as
        modified by other options.

   -u variable
   --unset=variable
        Unset variable VARIABLE (remove it from the environment).
        If VARIABLE was not set, does nothing.

   variable=value (an arg containing a "=" character)
        Set the environment variable VARIABLE to value VALUE.  VALUE
        may be of zero length ("variable=").  Setting a variable to a
        zero-length value is different from unsetting it.

   --
        Indicate that the following argument is the program
        to invoke.  This is necessary when the program's name
        begins with "-" or contains a "=".

   The first remaining argument specifies a program to invoke;
   it is searched for according to the specification of the PATH
   environment variable.  Any arguments following that are
   passed as arguments to that program.

   If no command name is specified following the environment
   specifications, the resulting environment is printed.
   This is like specifying a command name of "printenv".

   Examples:

   If the environment passed to "env" is
        { LOGNAME=rms EDITOR=emacs PATH=.:/gnubin:/hacks }

   env - foo
        runs "foo" in a null environment.

   env foo
        runs "foo" in the environment
        { LOGNAME=rms EDITOR=emacs PATH=.:/gnubin:/hacks }

   env DISPLAY=gnu:0 nemacs
        runs "nemacs" in the environment
        { LOGNAME=rms EDITOR=emacs PATH=.:/gnubin:/hacks DISPLAY=gnu:0 }

   env - LOGNAME=foo /hacks/hack bar baz
        runs the "hack" program on arguments "bar" and "baz" in an
        environment in which the only variable is "LOGNAME".  Note that
        the "-" option clears out the PATH variable, so one should be
        careful to specify in which directory to find the program to
        call.

   env -u EDITOR LOGNAME=foo PATH=/energy -- e=mc2 bar baz
        runs the program "/energy/e=mc2" with environment
        { LOGNAME=foo PATH=/energy }
*/

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"
#include "error.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "env"

#define AUTHORS \
  proper_name ("Richard Mlynarik"), \
  proper_name ("David MacKenzie")

extern char **environ;

static struct option const longopts[] =
{
  {"ignore-environment", no_argument, NULL, 'i'},
  {"unset", required_argument, NULL, 'u'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
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
      printf (_("\
Usage: %s [OPTION]... [-] [NAME=VALUE]... [COMMAND [ARG]...]\n"),
              program_name);
      fputs (_("\
Set each NAME to VALUE in the environment and run COMMAND.\n\
\n\
  -i, --ignore-environment   start with an empty environment\n\
  -u, --unset=NAME           remove variable from the environment\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
A mere - implies -i.  If no COMMAND, print the resulting environment.\n\
"), stdout);
      emit_bug_reporting_address ();
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int optc;
  bool ignore_environment = false;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (EXIT_FAILURE);
  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, "+iu:", longopts, NULL)) != -1)
    {
      switch (optc)
        {
        case 'i':
          ignore_environment = true;
          break;
        case 'u':
          break;
        case_GETOPT_HELP_CHAR;
        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
        default:
          usage (EXIT_FAILURE);
        }
    }

  if (optind < argc && STREQ (argv[optind], "-"))
    ignore_environment = true;

  if (ignore_environment)
    {
      static char *dummy_environ[] = { NULL };
      environ = dummy_environ;
    }

  optind = 0;			/* Force GNU getopt to re-initialize. */
  while ((optc = getopt_long (argc, argv, "+iu:", longopts, NULL)) != -1)
    if (optc == 'u')
      putenv (optarg);		/* Requires GNU putenv. */

  if (optind < argc && STREQ (argv[optind], "-"))
    ++optind;

  while (optind < argc && strchr (argv[optind], '='))
    putenv (argv[optind++]);

  /* If no program is specified, print the environment and exit. */
  if (argc <= optind)
    {
      char *const *e = environ;
      while (*e)
        puts (*e++);
      exit (EXIT_SUCCESS);
    }

  execvp (argv[optind], &argv[optind]);

  {
    int exit_status = (errno == ENOENT ? EXIT_ENOENT : EXIT_CANNOT_INVOKE);
    error (0, errno, "%s", argv[optind]);
    exit (exit_status);
  }
}
