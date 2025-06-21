/* tty -- print the name of the terminal connected to standard input
   Copyright (C) 1990-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Displays "not a tty" if stdin is not a terminal.
   Displays nothing if -s option is given.
   Exit status 0 if stdin is a tty, 1 if not a tty, 2 if usage error,
   3 if write error.

   Written by David MacKenzie <djm@gnu.ai.mit.edu>.  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "quote.h"

/* Exit statuses.  */
enum
  {
    TTY_STDIN_NOTTY = 1,
    TTY_USAGE = 2,
    TTY_WRITE_ERROR = 3,
    TTY_TTYNAME_FAILURE = 4
  };

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "tty"

#define AUTHORS proper_name ("David MacKenzie")

/* If true, return an exit status but produce no output. */
static bool silent;

static struct option const longopts[] =
{
  {"silent", no_argument, nullptr, 's'},
  {"quiet", no_argument, nullptr, 's'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("Usage: %s [OPTION]...\n"), program_name);
      fputs (_("\
Print the file name of the terminal connected to standard input.\n\
\n\
  -s, --silent, --quiet   print nothing, only return an exit status\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int optc;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (TTY_WRITE_ERROR);
  atexit (close_stdout);

  silent = false;

  while ((optc = getopt_long (argc, argv, "s", longopts, nullptr)) != -1)
    {
      switch (optc)
        {
        case 's':
          silent = true;
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (TTY_USAGE);
        }
    }

  if (optind < argc)
    {
      error (0, 0, _("extra operand %s"), quote (argv[optind]));
      usage (TTY_USAGE);
    }

  if (silent)
    return isatty (STDIN_FILENO) ? EXIT_SUCCESS : TTY_STDIN_NOTTY;

  int status;
  char const *tty = ttyname (STDIN_FILENO);

  if (tty)
    status = EXIT_SUCCESS;
  else
    {
      int ttyname_err = errno;
      if (isatty (STDIN_FILENO))
        error (TTY_TTYNAME_FAILURE, ttyname_err, _("ttyname error"));
      tty = _("not a tty");
      status = TTY_STDIN_NOTTY;
    }

  puts (tty);
  return status;
}
