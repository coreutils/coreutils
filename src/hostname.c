/* hostname - set or print the name of current host system
   Copyright (C) 1994-1997, 1999-2002 Free Software Foundation, Inc.

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

/* Jim Meyering <meyering@comco.com> */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>

#include "system.h"
#include "closeout.h"
#include "long-options.h"
#include "error.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "hostname"

#define AUTHORS "Jim Meyering"

#if !defined(HAVE_SETHOSTNAME) && defined(HAVE_SYSINFO) && \
     defined (HAVE_SYS_SYSTEMINFO_H) && defined(HAVE_LIMITS_H)
# include <sys/systeminfo.h>

int
sethostname (name, namelen)
     char *name;
     int namelen;
{
  /* Using sysinfo() is the SVR4 mechanism to set a hostname. */
  int result;

  result = sysinfo (SI_SET_HOSTNAME, name, namelen);

  return (result == -1 ? result : 0);
}

# define HAVE_SETHOSTNAME 1  /* Now we have it... */
#endif

char *xgethostname ();

/* The name this program was run with. */
char *program_name;

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [NAME]\n\
  or:  %s OPTION\n\
Print or set the hostname of the current system.\n\
\n\
"),
             program_name, program_name);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  char *hostname;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      AUTHORS, usage);

#ifdef HAVE_SETHOSTNAME
  if (argc == 2)
    {
      int err;

      /* Set hostname to argv[1].  */
      err = sethostname (argv[1], strlen (argv[1]));
      if (err != 0)
	error (EXIT_FAILURE, errno, _("cannot set hostname to `%s'"), argv[1]);
      exit (EXIT_SUCCESS);
    }
#else
  if (argc == 2)
    error (EXIT_FAILURE, 0,
	   _("cannot set hostname; this system lacks the functionality"));
#endif

  if (argc == 1)
    {
      hostname = xgethostname ();
      if (hostname == NULL)
	error (EXIT_FAILURE, errno, _("cannot determine hostname"));
      printf ("%s\n", hostname);
    }
  else
    {
      error (2, 0, _("too many arguments"));
      usage (EXIT_FAILURE);
    }

  exit (EXIT_SUCCESS);
}
