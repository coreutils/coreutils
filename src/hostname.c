/* hostname - set or print the name of current host system
   Copyright (C) 1994 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Jim Meyering <meyering@comco.com> */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>

#include "system.h"
#include "version.h"
#include "long-options.h"

#if !defined(HAVE_SETHOSTNAME) && defined(HAVE_SYSINFO) && \
     defined (HAVE_SYS_SYSTEMINFO_H) && defined(HAVE_LIMITS_H)
#include <limits.h>
#include <sys/systeminfo.h>

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

#define HAVE_SETHOSTNAME 1  /* Now we have it... */
#endif

void error ();
char *xgethostname ();

/* The name this program was run with. */
char *program_name;

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("\
Usage: %s [NAME]\n\
  or:  %s OPTION\n\
\n\
  --help      display this help and exit\n\
  --version   output version information and exit\n\
"
             , program_name, program_name);
    }
  exit (status);
}

void
main (argc, argv)
     int argc;
     char **argv;
{
  char *hostname;

  program_name = argv[0];

  parse_long_options (argc, argv, "hostname", version_string, usage);

#ifdef HAVE_SETHOSTNAME
  if (argc == 2)
    {
      int err;

      /* Set hostname to argv[1].  */
      err = sethostname (argv[1], strlen (argv[1]));
      if (err != 0)
	error (1, errno, "%s", argv[1]);
      exit (0);
    }
#else
  if (argc == 2)
    error (1, 0, "cannot set hostname; this system lacks the functionality");
#endif

  if (argc == 1)
    {
      hostname = xgethostname ();
      if (hostname == NULL)
	error (1, errno, "cannot determine hostname");
      printf ("%s\n", hostname);
    }
  else
    {
      error (2, 0, "too many arguments");
      usage (1);
    }

  exit (0);
}
