/* nohup -- run a command immume to hangups, with output to a non-tty
   Copyright (C) 2003, 2004 Free Software Foundation, Inc.

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

/* Written by Jim Meyering  */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#include "system.h"

#include "error.h"
#include "long-options.h"
#include "path-concat.h"
#include "quote.h"
#include "cloexec.h"

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
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int fd;
  int saved_stderr_fd = -1;
  int stderr_isatty;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (NOHUP_FAILURE);
  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      usage, AUTHORS, (char const *) NULL);

  /* The above handles --help and --version.
     Now, handle `--'.  */
  if (argc > 1 && STREQ (argv[1], "--"))
    {
      --argc;
      ++argv;
    }

  if (argc <= 1)
    {
      error (0, 0, _("too few arguments"));
      usage (NOHUP_FAILURE);
    }

  /* If standard output is a tty, redirect it (appending) to a file.
     First try nohup.out, then $HOME/nohup.out.  */
  if (isatty (STDOUT_FILENO))
    {
      char *in_home = NULL;
      char const *file = "nohup.out";
      int flags = O_CREAT | O_WRONLY | O_APPEND;
      mode_t mode = S_IRUSR | S_IWUSR;
      int saved_errno;

      fd = open (file, flags, mode);
      if (fd == -1)
	{
	  saved_errno = errno;
	  in_home = path_concat (getenv ("HOME"), file, NULL);
	  fd = open (in_home, flags, mode);
	  if (fd == -1)
	    {
	      int saved_errno2 = errno;
	      error (0, saved_errno, _("failed to open %s"), quote (file));
	      error (0, saved_errno2, _("failed to open %s"), quote (in_home));
	      exit (NOHUP_FAILURE);
	    }
	  file = in_home;
	}

      /* Redirect standard output to the file.  */
      if (dup2 (fd, STDOUT_FILENO) == -1)
	error (NOHUP_FAILURE, errno, _("failed to redirect standard output"));

      error (0, 0, _("appending output to %s"), quote (file));
      if (in_home)
	free (in_home);
    }
  else
    {
      fd = STDOUT_FILENO;
    }

  /* If stderr is on a tty, redirect it to stdout.  */
  if ((stderr_isatty = isatty (STDERR_FILENO)))
    {
      /* Save a copy of stderr before redirecting, so we can use the original
	 if execve fails.  It's no big deal if this dup fails.  It might
	 not change anything, and at worst, it'll lead to suppression of
	 the post-failed-execve diagnostic.  */
      saved_stderr_fd = dup (STDERR_FILENO);

      if (saved_stderr_fd != -1
	  && ! set_cloexec_flag (saved_stderr_fd, true))
	error (NOHUP_FAILURE, errno,
	       _("failed to set the copy of stderr to close on exec"));

      if (dup2 (fd, STDERR_FILENO) == -1)
	error (NOHUP_FAILURE, errno, _("failed to redirect standard error"));
    }

  /* Ignore hang-up signals.  */
  {
#ifdef _POSIX_SOURCE
    struct sigaction sigact;
    sigact.sa_handler = SIG_IGN;
    sigemptyset (&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction (SIGHUP, &sigact, NULL);
#else
    signal (SIGHUP, SIG_IGN);
#endif
  }

  {
    int exit_status;
    int saved_errno;
    char **cmd = argv + 1;

    execvp (*cmd, cmd);
    exit_status = (errno == ENOENT ? EXIT_ENOENT : EXIT_CANNOT_INVOKE);
    saved_errno = errno;

    /* The execve failed.  Output a diagnostic to stderr only if:
       - stderr was initially redirected to a non-tty, or
       - stderr was initially directed to a tty, and we've
	 just dup2'd it to point back to that same tty.
       In other words, output the diagnostic if possible, but not if
       it'd go to nohup.out.  */
    if ( ! stderr_isatty
	|| (saved_stderr_fd != -1
	    && dup2 (saved_stderr_fd, STDERR_FILENO) != -1))
      error (0, saved_errno, _("cannot run command %s"), quote (*cmd));

    exit (exit_status);
  }
}
