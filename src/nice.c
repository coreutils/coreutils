/* nice -- run a program with modified scheduling priority
   Copyright (C) 1990, 1991 Free Software Foundation, Inc.

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

/* David MacKenzie <djm@gnu.ai.mit.edu> */

#ifdef HAVE_CONFIG_H
#if defined (CONFIG_BROKETS)
/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because it found this file in $srcdir).  */
#include <config.h>
#else
#include "config.h"
#endif
#endif

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#ifndef NICE_PRIORITY
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include "version.h"
#include "system.h"

#ifdef NICE_PRIORITY
#define GET_PRIORITY() nice (0)
#else
#define GET_PRIORITY() getpriority (PRIO_PROCESS, 0)
#endif

void error ();

static int isinteger ();
static void usage ();

/* The name this program was run with. */
char *program_name;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output and exit.  */
static int show_version;

static struct option const longopts[] =
{
  {"adjustment", required_argument, NULL, 'n'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

void
main (argc, argv)
     int argc;
     char **argv;
{
  int current_priority;
  int adjustment = 0;
  int minusflag = 0;
  int adjustment_given = 0;
  int optc;

  program_name = argv[0];

  while ((optc = getopt_long (argc, argv, "+0123456789-n:", longopts,
			      (int *) 0)) != EOF)
    {
      switch (optc)
	{
	case '?':
	  usage (1);

	case 0:
	  break;
	  
	case 'n':
	  if (!isinteger (optarg))
	    error (1, 0, "invalid priority `%s'", optarg);
	  adjustment = atoi (optarg);
	  adjustment_given = 1;
	  break;

	case '-':
	  minusflag = 1;
	  break;

	default:
	  adjustment = adjustment * 10 + optc - '0';
	  adjustment_given = 1;
	}
    }

  if (show_version)
    {
      printf ("%s\n", version_string);
      exit (0);
    }

  if (show_help)
    usage (0);

  if (minusflag)
    adjustment = -adjustment;
  if (!adjustment_given)
    adjustment = 10;

  if (optind == argc)
    {
      if (adjustment_given)
	{
	  error (0, 0, "a command must be given with an adjustment");
	  usage (1);
	}
      /* No command given; print the priority. */
      errno = 0;
      current_priority = GET_PRIORITY ();
      if (current_priority == -1 && errno != 0)
	error (1, errno, "cannot get priority");
      printf ("%d\n", current_priority);
      exit (0);
    }

#ifndef NICE_PRIORITY
  errno = 0;
  current_priority = GET_PRIORITY ();
  if (current_priority == -1 && errno != 0)
    error (1, errno, "cannot get priority");
  if (setpriority (PRIO_PROCESS, 0, current_priority + adjustment))
#else
  if (nice (adjustment) == -1)
#endif
    error (1, errno, "cannot set priority");

  execvp (argv[optind], &argv[optind]);
  error (errno == ENOENT ? 127 : 126, errno, "%s", argv[optind]);
}

/* Return nonzero if S represents a (possibly signed) decimal integer,
   zero if not. */

static int
isinteger (s)
     char *s;
{
  if (*s == '-' || *s == '+')
    ++s;
  if (*s == 0)
    return 0;
  while (*s)
    {
      if (*s < '0' || *s > '9')
	return 0;
      ++s;
    }
  return 1;
}

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("Usage: %s [OPTION]... [COMMAND [ARG]...]\n", program_name);
      printf ("\
\n\
  -ADJUST                   increment priority by ADJUST first\n\
  -n, --adjustment=ADJUST   same as -ADJUST\n\
      --help                display this help and exit\n\
      --version             output version information and exit\n\
\n\
With no COMMAND, print the current scheduling priority.  ADJUST is 10\n\
by default.  Range goes from -20 (highest priority) to 19 (lowest).\n\
");
    }
  exit (status);
}
