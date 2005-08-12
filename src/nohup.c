/* nohup -- run a command immune to hangups, with output to a non-tty
   Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.

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

/* Written by Jim Meyering  */

#include <config.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#include "system.h"

#include "cloexec.h"
#include "error.h"
#include "filenamecat.h"
#include "fd-reopen.h"
#include "long-options.h"
#include "quote.h"
#include "unistd--.h"

#define PROGRAM_NAME "nohup"

#define AUTHORS "Jim Meyering"

/* Exit statuses.  */
enum
  {
    /* `nohup' itself failed.  */
    NOHUP_FAILURE = 127
  };

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
Usage: %s COMMAND [ARG]...\n\
  or:  %s OPTION\n\
"),
	      program_name, program_name);

      fputs (_("\
Run COMMAND, ignoring hangup signals.\n\
\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (USAGE_BUILTIN_WARNING, PROGRAM_NAME);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int saved_stderr_fd = STDERR_FILENO;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (NOHUP_FAILURE);
  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      usage, AUTHORS, (char const *) NULL);
  if (getopt_long (argc, argv, "+", NULL, NULL) != -1)
    usage (NOHUP_FAILURE);

  if (argc <= optind)
    {
      error (0, 0, _("missing operand"));
      usage (NOHUP_FAILURE);
    }

  /* If standard input is a tty, replace it with /dev/null.
     Note that it is deliberately opened for *writing*,
     to ensure any read evokes an error.  */
  if (isatty (STDIN_FILENO))
    fd_reopen (STDIN_FILENO, "/dev/null", O_WRONLY, 0);

  /* If standard output is a tty, redirect it (appending) to a file.
     First try nohup.out, then $HOME/nohup.out.  */
  if (isatty (STDOUT_FILENO))
    {
      char *in_home = NULL;
      char const *file = "nohup.out";
      int flags = O_CREAT | O_WRONLY | O_APPEND;
      mode_t mode = S_IRUSR | S_IWUSR;
      mode_t umask_value = umask (~mode);
      int fd = fd_reopen (STDOUT_FILENO, file, flags, mode);

      if (fd < 0)
	{
	  int saved_errno = errno;
	  char const *home = getenv ("HOME");
	  if (home)
	    {
	      in_home = file_name_concat (home, file, NULL);
	      fd = fd_reopen (STDOUT_FILENO, in_home, flags, mode);
	    }
	  if (fd < 0)
	    {
	      int saved_errno2 = errno;
	      error (0, saved_errno, _("failed to open %s"), quote (file));
	      if (in_home)
		error (0, saved_errno2, _("failed to open %s"),
		       quote (in_home));
	      exit (NOHUP_FAILURE);
	    }
	  file = in_home;
	}

      umask (umask_value);
      error (0, 0, _("appending output to %s"), quote (file));
      free (in_home);
    }

  /* If standard error is a tty, redirect it to stdout.  */
  if (isatty (STDERR_FILENO))
    {
      /* Save a copy of stderr before redirecting, so we can use the original
	 if execve fails.  It's no big deal if this dup fails.  It might
	 not change anything, and at worst, it'll lead to suppression of
	 the post-failed-execve diagnostic.  */
      saved_stderr_fd = dup (STDERR_FILENO);

      if (0 <= saved_stderr_fd
	  && set_cloexec_flag (saved_stderr_fd, true) != 0)
	error (NOHUP_FAILURE, errno,
	       _("failed to set the copy of stderr to close on exec"));

      if (dup2 (STDOUT_FILENO, STDERR_FILENO) < 0)
	{
	  if (errno != EBADF)
	    error (NOHUP_FAILURE, errno,
		   _("failed to redirect standard error"));
	  close (STDERR_FILENO);
	}
    }

  signal (SIGHUP, SIG_IGN);

  {
    int exit_status;
    int saved_errno;
    char **cmd = argv + optind;

    execvp (*cmd, cmd);
    exit_status = (errno == ENOENT ? EXIT_ENOENT : EXIT_CANNOT_INVOKE);
    saved_errno = errno;

    /* The execve failed.  Output a diagnostic to stderr only if:
       - stderr was initially redirected to a non-tty, or
       - stderr was initially directed to a tty, and we
	 can dup2 it to point back to that same tty.
       In other words, output the diagnostic if possible, but only if
       it will go to the original stderr.  */
    if (dup2 (saved_stderr_fd, STDERR_FILENO) == STDERR_FILENO)
      error (0, saved_errno, _("cannot run command %s"), quote (*cmd));

    exit (exit_status);
  }
}
