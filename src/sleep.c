/* sleep - delay for a specified amount of time.
   Copyright (C) 84, 1991-1997, 1999 Free Software Foundation, Inc.

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

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"
#include "error.h"
#include "long-options.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "sleep"

#define AUTHORS "FIXME: unknown"

static long argdecode PARAMS ((const char *s));

/* The name by which this program was run. */
char *program_name;

static struct option const long_options[] =
{
  {0, 0, 0, 0}
};

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... NUMBER[SUFFIX]\n"), program_name);
      printf (_("\
Pause for NUMBER seconds.\n\
SUFFIX may be s to keep seconds, m for minutes, h for hours or d for days.\n\
\n\
  --help      display this help and exit\n\
  --version   output version information and exit\n"));
      puts (_("\nReport bugs to <bug-sh-utils@gnu.org>."));
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int i;
  unsigned seconds = 0;
  int c;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      AUTHORS, usage);

  while ((c = getopt_long (argc, argv, "", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	default:
	  usage (1);
	}
    }

  if (argc == 1)
    {
      error (0, 0, _("too few arguments"));
      usage (1);
    }

  for (i = 1; i < argc; i++)
    seconds += argdecode (argv[i]);

  sleep (seconds);

  exit (0);
}

static long
argdecode (const char *s)
{
  long value;
  register const char *p = s;
  register char c;

  value = 0;
  while ((c = *p++) >= '0' && c <= '9')
    value = value * 10 + c - '0';

  switch (c)
    {
    case 's':
      break;
    case 'm':
      value *= 60;
      break;
    case 'h':
      value *= 60 * 60;
      break;
    case 'd':
      value *= 60 * 60 * 24;
      break;
    default:
      p--;
    }

  if (*p)
    error (1, 0, _("invalid time interval `%s'"), s);
  return value;
}
