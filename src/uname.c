/* uname -- print system information

   Copyright 1989, 1992, 1993, 1996, 1997, 1999, 2000, 2001 Free
   Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu> */

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
#include "closeout.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "uname"

#define AUTHORS "David MacKenzie"

/* Values that are bitwise or'd into `toprint'. */
/* Kernel name. */
#define PRINT_KERNEL_NAME 1

/* Node name on a communications network. */
#define PRINT_NODENAME 2

/* Kernel release. */
#define PRINT_KERNEL_RELEASE 4

/* Kernel version. */
#define PRINT_KERNEL_VERSION 8

/* Machine hardware name. */
#define PRINT_MACHINE 16

/* Processor type. */
#define PRINT_PROCESSOR 32

/* Hardware platform.  */
#define PRINT_HARDWARE_PLATFORM 64

/* Operating system.  */
#define PRINT_OPERATING_SYSTEM 128

/* The name this program was run with, for error messages. */
char *program_name;

static struct option const long_options[] =
{
  {"all", no_argument, NULL, 'a'},
  {"kernel-name", no_argument, NULL, 's'},
  {"sysname", no_argument, NULL, 's'},	/* Obsolescent.  */
  {"nodename", no_argument, NULL, 'n'},
  {"kernel-release", no_argument, NULL, 'r'},
  {"release", no_argument, NULL, 'r'},  /* Obsolescent.  */
  {"kernel-version", no_argument, NULL, 'v'},
  {"machine", no_argument, NULL, 'm'},
  {"processor", no_argument, NULL, 'p'},
  {"hardware-platform", no_argument, NULL, 'i'},
  {"operating-system", no_argument, NULL, 'o'},
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
      fputs (_("\
Print certain system information.  With no OPTION, same as -s.\n\
\n\
  -a, --all                print all information, in the following order:\n\
  -s, --kernel-name        print the kernel name\n\
  -n, --nodename           print the network node hostname\n\
  -r, --kernel-release     print the kernel release\n\
"), stdout);
      fputs (_("\
  -v, --kernel-version     print the kernel version\n\
  -m, --machine            print the machine hardware name\n\
  -p, --processor          print the processor type\n\
  -i, --hardware-platform  print the hardware platform\n\
  -o, --operating-system   print the operating system\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      puts (_("\nReport bugs to <bug-sh-utils@gnu.org>."));
    }
  exit (status);
}

/* Print ELEMENT, preceded by a space if something has already been
   printed.  */

static void
print_element (char const *element)
{
  static int printed;
  if (printed++)
    putchar (' ');
  fputs (element, stdout);
}

int
main (int argc, char **argv)
{
  int c;

  /* Mask indicating which elements to print. */
  unsigned toprint = 0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((c = getopt_long (argc, argv, "asnrvmpio", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case 'a':
	  toprint = -1;
	  break;

	case 's':
	  toprint |= PRINT_KERNEL_NAME;
	  break;

	case 'n':
	  toprint |= PRINT_NODENAME;
	  break;

	case 'r':
	  toprint |= PRINT_KERNEL_RELEASE;
	  break;

	case 'v':
	  toprint |= PRINT_KERNEL_VERSION;
	  break;

	case 'm':
	  toprint |= PRINT_MACHINE;
	  break;

	case 'p':
	  toprint |= PRINT_PROCESSOR;
	  break;

	case 'i':
	  toprint |= PRINT_HARDWARE_PLATFORM;
	  break;

	case 'o':
	  toprint |= PRINT_OPERATING_SYSTEM;
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
    toprint = PRINT_KERNEL_NAME;

  if (toprint
       & (PRINT_KERNEL_NAME | PRINT_NODENAME | PRINT_KERNEL_RELEASE
	  | PRINT_KERNEL_VERSION | PRINT_MACHINE))
    {
      struct utsname name;

      if (uname (&name) == -1)
	error (1, errno, _("cannot get system name"));

      if (toprint & PRINT_KERNEL_NAME)
	print_element (name.sysname);
      if (toprint & PRINT_NODENAME)
	print_element (name.nodename);
      if (toprint & PRINT_KERNEL_RELEASE)
	print_element (name.release);
      if (toprint & PRINT_KERNEL_VERSION)
	print_element (name.version);
      if (toprint & PRINT_MACHINE)
	print_element (name.machine);
    }

  if (toprint & PRINT_PROCESSOR)
    {
      char const *element = "unknown";
#if defined (HAVE_SYSINFO) && defined (SI_ARCHITECTURE)
      char processor[257];
      if (0 <= sysinfo (SI_ARCHITECTURE, processor, sizeof processor))
	element = processor;
#endif
      print_element (element);
    }

  if (toprint & PRINT_HARDWARE_PLATFORM)
    {
      char const *element = "unknown";
#if defined (HAVE_SYSINFO) && defined (SI_PLATFORM)
      char hardware_platform[257];
      if (0 <= sysinfo (SI_PLATFORM,
			hardware_platform, sizeof hardware_platform))
	element = hardware_platform;
#endif
      print_element (element);
    }

  if (toprint & PRINT_OPERATING_SYSTEM)
    print_element (HOST_OPERATING_SYSTEM);

  putchar ('\n');

  exit (0);
}
