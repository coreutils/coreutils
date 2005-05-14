/* mknod -- make special files
   Copyright (C) 90, 91, 1995-2005 Free Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@ai.mit.edu>  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "error.h"
#include "modechange.h"
#include "quote.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "mknod"

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

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... NAME TYPE [MAJOR MINOR]\n"),
	      program_name);
      fputs (_("\
Create the special file NAME of the given TYPE.\n\
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
      fputs (_("\
\n\
Both MAJOR and MINOR must be specified when TYPE is b, c, or u, and they\n\
must be omitted when TYPE is p.  If MAJOR or MINOR begins with 0x or 0X,\n\
it is interpreted as hexadecimal; otherwise, if it begins with 0, as octal;\n\
otherwise, as decimal.  TYPE may be:\n\
"), stdout);
      fputs (_("\
\n\
  b      create a block (buffered) special file\n\
  c, u   create a character (unbuffered) special file\n\
  p      create a FIFO\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  mode_t newmode;
  const char *specified_mode;
  int optc;
  int expected_operands;
  mode_t node_type;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  specified_mode = NULL;

  while ((optc = getopt_long (argc, argv, "m:", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 'm':
	  specified_mode = optarg;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (EXIT_FAILURE);
	}
    }

  newmode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (specified_mode)
    {
      struct mode_change *change = mode_compile (specified_mode);
      if (!change)
	error (EXIT_FAILURE, 0, _("invalid mode"));
      newmode = mode_adjust (newmode, change, umask (0));
      free (change);
    }

  /* If the number of arguments is 0 or 1,
     or (if it's 2 or more and the second one starts with `p'), then there
     must be exactly two operands.  Otherwise, there must be four.  */
  expected_operands = (argc <= optind
		       || (optind + 1 < argc && argv[optind + 1][0] == 'p')
		       ? 2 : 4);

  if (argc - optind < expected_operands)
    {
      if (argc <= optind)
	error (0, 0, _("missing operand"));
      else
	error (0, 0, _("missing operand after %s"), quote (argv[argc - 1]));
      if (expected_operands == 4 && argc - optind == 2)
	fprintf (stderr, "%s\n",
		 _("Special files require major and minor device numbers."));
      usage (EXIT_FAILURE);
    }

  if (expected_operands < argc - optind)
    {
      error (0, 0, _("extra operand %s"),
	     quote (argv[optind + expected_operands]));
      if (expected_operands == 2 && argc - optind == 4)
	fprintf (stderr, "%s\n",
		 _("Fifos do not have major and minor device numbers."));
      usage (EXIT_FAILURE);
    }

  /* Only check the first character, to allow mnemonic usage like
     `mknod /dev/rst0 character 18 0'. */

  switch (argv[optind + 1][0])
    {
    case 'b':			/* `block' or `buffered' */
#ifndef S_IFBLK
      error (EXIT_FAILURE, 0, _("block special files not supported"));
#else
      node_type = S_IFBLK;
#endif
      goto block_or_character;

    case 'c':			/* `character' */
    case 'u':			/* `unbuffered' */
#ifndef S_IFCHR
      error (EXIT_FAILURE, 0, _("character special files not supported"));
#else
      node_type = S_IFCHR;
#endif
      goto block_or_character;

    block_or_character:
      {
	char const *s_major = argv[optind + 2];
	char const *s_minor = argv[optind + 3];
	uintmax_t i_major, i_minor;
	dev_t device;

	if (xstrtoumax (s_major, NULL, 0, &i_major, NULL) != LONGINT_OK
	    || i_major != (major_t) i_major)
	  error (EXIT_FAILURE, 0,
		 _("invalid major device number %s"), quote (s_major));

	if (xstrtoumax (s_minor, NULL, 0, &i_minor, NULL) != LONGINT_OK
	    || i_minor != (minor_t) i_minor)
	  error (EXIT_FAILURE, 0,
		 _("invalid minor device number %s"), quote (s_minor));

	device = makedev (i_major, i_minor);
#ifdef NODEV
	if (device == NODEV)
	  error (EXIT_FAILURE, 0, _("invalid device %s %s"), s_major, s_minor);
#endif

	if (mknod (argv[optind], newmode | node_type, device) != 0)
	  error (EXIT_FAILURE, errno, "%s", quote (argv[optind]));
      }
      break;

    case 'p':			/* `pipe' */
#ifndef S_ISFIFO
      error (EXIT_FAILURE, 0, _("fifo files not supported"));
#else
      if (mkfifo (argv[optind], newmode))
	error (EXIT_FAILURE, errno, "%s", quote (argv[optind]));
#endif
      break;

    default:
      error (0, 0, _("invalid device type %s"), quote (argv[optind + 1]));
      usage (EXIT_FAILURE);
    }

  /* Perform an explicit chmod to ensure the file mode permission bits
     are set as specified.  This extra step is necessary in some cases
     when the containing directory has a default ACL.  */

  if (specified_mode)
    {
      if (chmod (argv[optind], newmode))
        error (0, errno, _("cannot set permissions of %s"),
               quote (argv[optind]));
    }

  exit (EXIT_SUCCESS);
}
