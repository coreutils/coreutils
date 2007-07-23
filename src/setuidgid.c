/* setuidgid - run a command with the UID and GID of a specified user
   Copyright (C) 2003-2007 Free Software Foundation, Inc.

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

/* Written by Jim Meyering  */

#include <config.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include "system.h"

#include "error.h"
#include "long-options.h"
#include "mgetgroups.h"
#include "quote.h"

#define PROGRAM_NAME "setuidgid"

/* I wrote this program from scratch, based on the description of
   D.J. Bernstein's program: http://cr.yp.to/daemontools/setuidgid.html.  */
#define AUTHORS "Jim Meyering"

#define SETUIDGID_FAILURE 111

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
Usage: %s USERNAME COMMAND [ARGUMENT]...\n\
  or:  %s OPTION\n\
"),
	      program_name, program_name);

      fputs (_("\
Drop any supplemental groups, assume the user-ID and group-ID of\n\
the specified USERNAME, and run COMMAND with any specified ARGUMENTs.\n\
Exit with status 111 if unable to assume the required user and group ID.\n\
Otherwise, exit with the exit status of COMMAND.\n\
This program is useful only when run by root (user ID zero).\n\
\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_bug_reporting_address ();
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  char const *user_id;
  struct passwd *pwd;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (SETUIDGID_FAILURE);
  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      usage, AUTHORS, (char const *) NULL);
  if (getopt_long (argc, argv, "+", NULL, NULL) != -1)
    usage (SETUIDGID_FAILURE);

  if (argc <= optind + 1)
    {
      if (argc < optind + 1)
	error (0, 0, _("missing operand"));
      else
	error (0, 0, _("missing operand after %s"), quote (argv[optind]));
      usage (SETUIDGID_FAILURE);
    }

  user_id = argv[optind];
  pwd = getpwnam (user_id);
  if (pwd == NULL)
    error (SETUIDGID_FAILURE, errno,
	   _("unknown user-ID: %s"), quote (user_id));

#if HAVE_SETGROUPS
  {
    GETGROUPS_T *groups;
    int n_groups = mgetgroups (user_id, pwd->pw_gid, &groups);
    if (n_groups < 0)
      {
	n_groups = 1;
	groups = xmalloc (sizeof *groups);
	*groups = pwd->pw_gid;
      }

    if (0 < n_groups && setgroups (n_groups, groups))
      error (SETUIDGID_FAILURE, errno, _("cannot set supplemental group(s)"));

    free (groups);
  }
#endif

  if (setgid (pwd->pw_gid))
    error (SETUIDGID_FAILURE, errno,
	   _("cannot set group-ID to %lu"), (unsigned long int) pwd->pw_gid);

  if (setuid (pwd->pw_uid))
    error (SETUIDGID_FAILURE, errno,
	   _("cannot set user-ID to %lu"), (unsigned long int) pwd->pw_uid);

  {
    char **cmd = argv + optind + 1;
    int exit_status;
    execvp (*cmd, cmd);
    exit_status = (errno == ENOENT ? EXIT_ENOENT : EXIT_CANNOT_INVOKE);

    error (0, errno, _("cannot run command %s"), quote (*cmd));
    exit (exit_status);
  }
}
