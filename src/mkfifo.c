/* mkfifo -- make fifo's (named pipes)
   Copyright (C) 90, 91, 1995-2002 Free Software Foundation, Inc.

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

/* David MacKenzie <djm@ai.mit.edu>  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "error.h"
#include "modechange.h"
#include "quote.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "mkfifo"

#define AUTHORS "David MacKenzie"

/* The name this program was run with. */
char *program_name;

static struct option const longopts[] =
{
  {"mode", required_argument, NULL, 'm'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

#ifdef S_ISFIFO
void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION] NAME...\n"), program_name);
      fputs (_("\
Create named pipes (FIFOs) with the given NAMEs.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -m, --mode=MODE   set permission mode (as in chmod), not a=rw - umask\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}
#endif

int
main (int argc, char **argv)
{
  mode_t newmode;
  struct mode_change *change;
  const char *specified_mode;
  int errors = 0;
  int optc;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  specified_mode = NULL;

#ifndef S_ISFIFO
  error (4, 0, _("fifo files not supported"));
#else
  while ((optc = getopt_long (argc, argv, "m:", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;
	case 'm':
	  specified_mode = optarg;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (optind == argc)
    {
      error (0, 0, _("too few arguments"));
      usage (EXIT_FAILURE);
    }

  newmode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (specified_mode)
    {
      newmode &= ~ umask (0);
      change = mode_compile (specified_mode, 0);
      if (change == MODE_INVALID)
	error (EXIT_FAILURE, 0, _("invalid mode"));
      else if (change == MODE_MEMORY_EXHAUSTED)
	xalloc_die ();
      newmode = mode_adjust (newmode, change);
    }

  for (; optind < argc; ++optind)
    {
      int fail = mkfifo (argv[optind], newmode);
      if (fail)
	error (0, errno, _("cannot create fifo %s"), quote (argv[optind]));

      /* If the containing directory happens to have a default ACL, chmod
	 ensures the file mode permission bits are still set as desired.  */

      if (fail == 0 && specified_mode)
 	{
 	  fail = chmod (argv[optind], newmode);
	  if (fail)
	    error (0, errno, _("cannot set permissions of fifo %s"),
		   quote (argv[optind]));
 	}

      if (fail)
 	errors = 1;
    }

  exit (errors);
#endif
}
