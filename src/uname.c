/* uname -- print system information
   Copyright (C) 1989-1999 Free Software Foundation, Inc.

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

/* Option		Example

   -s, --sysname	SunOS
   -n, --nodename	rocky8
   -r, --release	4.0
   -v, --version
   -m, --machine	sun
   -a, --all		SunOS rocky8 4.0  sun

   The default behavior is equivalent to `-s'.

   David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <getopt.h>

#if defined (HAVE_SYSINFO) && defined (HAVE_SYS_SYSTEMINFO_H)
# include <sys/systeminfo.h>
#endif

#include "system.h"
#include "error.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "uname"

#define AUTHORS "David MacKenzie"

static void print_element PARAMS ((unsigned int mask, char *element));

/* Values that are bitwise or'd into `toprint'. */
/* Operating system name. */
#define PRINT_SYSNAME 1

/* Node name on a communications network. */
#define PRINT_NODENAME 2

/* Operating system release. */
#define PRINT_RELEASE 4

/* Operating system version. */
#define PRINT_VERSION 8

/* Machine hardware name. */
#define PRINT_MACHINE 16

 /* Host processor type. */
#define PRINT_PROCESSOR 32

/* Mask indicating which elements of the name to print. */
static unsigned char toprint;

/* The name this program was run with, for error messages. */
char *program_name;

static struct option const long_options[] =
{
  {"machine", no_argument, NULL, 'm'},
  {"nodename", no_argument, NULL, 'n'},
  {"release", no_argument, NULL, 'r'},
  {"sysname", no_argument, NULL, 's'},
  {"processor", no_argument, NULL, 'p'},
  {"all", no_argument, NULL, 'a'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]...\n"), program_name);
      printf (_("\
Print certain system information.  With no OPTION, same as -s.\n\
\n\
  -a, --all        print all information\n\
  -m, --machine    print the machine (hardware) type\n\
  -n, --nodename   print the machine's network node hostname\n\
  -r, --release    print the operating system release\n\
  -s, --sysname    print the operating system name\n\
  -p, --processor  print the host processor type\n\
  -v               print the operating system version\n\
      --help       display this help and exit\n\
      --version    output version information and exit\n"));
      puts (_("\nReport bugs to <bug-sh-utils@gnu.org>."));
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  struct utsname name;
  int c;
  char processor[256];

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  toprint = 0;

  while ((c = getopt_long (argc, argv, "snrvpma", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case 's':
	  toprint |= PRINT_SYSNAME;
	  break;

	case 'n':
	  toprint |= PRINT_NODENAME;
	  break;

	case 'r':
	  toprint |= PRINT_RELEASE;
	  break;

	case 'v':
	  toprint |= PRINT_VERSION;
	  break;

	case 'm':
	  toprint |= PRINT_MACHINE;
	  break;

	case 'p':
	  toprint |= PRINT_PROCESSOR;
	  break;

	case 'a':
	  toprint = (PRINT_SYSNAME | PRINT_NODENAME | PRINT_RELEASE |
		     PRINT_PROCESSOR | PRINT_VERSION | PRINT_MACHINE);
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (1);
	}
    }

  if (optind != argc)
    usage (1);

  if (toprint == 0)
    toprint = PRINT_SYSNAME;

  if (uname (&name) == -1)
    error (1, errno, _("cannot get system name"));

#if defined (HAVE_SYSINFO) && defined (SI_ARCHITECTURE)
  if (sysinfo (SI_ARCHITECTURE, processor, sizeof (processor)) == -1)
    error (1, errno, _("cannot get processor type"));
#else
  strcpy (processor, "unknown");
#endif

  print_element (PRINT_SYSNAME, name.sysname);
  print_element (PRINT_NODENAME, name.nodename);
  print_element (PRINT_RELEASE, name.release);
  print_element (PRINT_VERSION, name.version);
  print_element (PRINT_MACHINE, name.machine);
  print_element (PRINT_PROCESSOR, processor);

  exit (0);
}

/* If the name element set in MASK is selected for printing in `toprint',
   print ELEMENT; then print a space unless it is the last element to
   be printed, in which case print a newline. */

static void
print_element (unsigned int mask, char *element)
{
  if (toprint & mask)
    {
      toprint &= ~mask;
      printf ("%s%c", element, toprint ? ' ' : '\n');
    }
}
