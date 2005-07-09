/* comm -- compare two sorted files line by line.
   Copyright (C) 86, 90, 91, 1995-2005 Free Software Foundation, Inc.

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

/* Written by Richard Stallman and David MacKenzie. */

#include <config.h>

#include <getopt.h>
#include <sys/types.h>
#include "system.h"
#include "linebuffer.h"
#include "error.h"
#include "hard-locale.h"
#include "quote.h"
#include "stdio--.h"
#include "xmemcoll.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "comm"

#define AUTHORS "Richard Stallman", "David MacKenzie"

/* Undefine, to avoid warning about redefinition on some systems.  */
#undef min
#define min(x, y) ((x) < (y) ? (x) : (y))

/* The name this program was run with. */
char *program_name;

/* True if the LC_COLLATE locale is hard.  */
static bool hard_LC_COLLATE;

/* If true, print lines that are found only in file 1. */
static bool only_file_1;

/* If true, print lines that are found only in file 2. */
static bool only_file_2;

/* If true, print lines that are found in both files. */
static bool both;

static struct option const long_options[] =
{
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
      printf (_("\
Usage: %s [OPTION]... FILE1 FILE2\n\
"),
	      program_name);
      fputs (_("\
Compare sorted files FILE1 and FILE2 line by line.\n\
"), stdout);
      fputs (_("\
\n\
With no options, produce three-column output.  Column one contains\n\
lines unique to FILE1, column two contains lines unique to FILE2,\n\
and column three contains lines common to both files.\n\
"), stdout);
      fputs (_("\
\n\
  -1              suppress lines unique to FILE1\n\
  -2              suppress lines unique to FILE2\n\
  -3              suppress lines that appear in both files\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

/* Output the line in linebuffer LINE to stream STREAM
   provided the switches say it should be output.
   CLASS is 1 for a line found only in file 1,
   2 for a line only in file 2, 3 for a line in both. */

static void
writeline (const struct linebuffer *line, FILE *stream, int class)
{
  switch (class)
    {
    case 1:
      if (!only_file_1)
	return;
      break;

    case 2:
      if (!only_file_2)
	return;
      /* Print a TAB if we are printing lines from file 1.  */
      if (only_file_1)
	putc ('\t', stream);
      break;

    case 3:
      if (!both)
	return;
      /* Print a TAB if we are printing lines from file 1.  */
      if (only_file_1)
	putc ('\t', stream);
      /* Print a TAB if we are printing lines from file 2.  */
      if (only_file_2)
	putc ('\t', stream);
      break;
    }

  fwrite (line->buffer, sizeof (char), line->length, stream);
}

/* Compare INFILES[0] and INFILES[1].
   If either is "-", use the standard input for that file.
   Assume that each input file is sorted;
   merge them and output the result.  */

static void
compare_files (char **infiles)
{
  /* For each file, we have one linebuffer in lb1.  */
  struct linebuffer lb1[2];

  /* thisline[i] points to the linebuffer holding the next available line
     in file i, or is NULL if there are no lines left in that file.  */
  struct linebuffer *thisline[2];

  /* streams[i] holds the input stream for file i.  */
  FILE *streams[2];

  int i;

  /* Initialize the storage. */
  for (i = 0; i < 2; i++)
    {
      initbuffer (&lb1[i]);
      thisline[i] = &lb1[i];
      streams[i] = (STREQ (infiles[i], "-") ? stdin : fopen (infiles[i], "r"));
      if (!streams[i])
	error (EXIT_FAILURE, errno, "%s", infiles[i]);

      thisline[i] = readlinebuffer (thisline[i], streams[i]);
      if (ferror (streams[i]))
	error (EXIT_FAILURE, errno, "%s", infiles[i]);
    }

  while (thisline[0] || thisline[1])
    {
      int order;

      /* Compare the next available lines of the two files.  */

      if (!thisline[0])
	order = 1;
      else if (!thisline[1])
	order = -1;
      else
	{
	  if (hard_LC_COLLATE)
	    order = xmemcoll (thisline[0]->buffer, thisline[0]->length - 1,
			      thisline[1]->buffer, thisline[1]->length - 1);
	  else
	    {
	      size_t len = min (thisline[0]->length, thisline[1]->length) - 1;
	      order = memcmp (thisline[0]->buffer, thisline[1]->buffer, len);
	      if (order == 0)
		order = (thisline[0]->length < thisline[1]->length
			 ? -1
			 : thisline[0]->length != thisline[1]->length);
	    }
	}

      /* Output the line that is lesser. */
      if (order == 0)
	writeline (thisline[1], stdout, 3);
      else if (order > 0)
	writeline (thisline[1], stdout, 2);
      else
	writeline (thisline[0], stdout, 1);

      /* Step the file the line came from.
	 If the files match, step both files.  */
      if (order >= 0)
	{
	  thisline[1] = readlinebuffer (thisline[1], streams[1]);
	  if (ferror (streams[1]))
	    error (EXIT_FAILURE, errno, "%s", infiles[1]);
	}
      if (order <= 0)
	{
	  thisline[0] = readlinebuffer (thisline[0], streams[0]);
	  if (ferror (streams[0]))
	    error (EXIT_FAILURE, errno, "%s", infiles[0]);
	}
    }

  for (i = 0; i < 2; i++)
    if (fclose (streams[i]) != 0)
      error (EXIT_FAILURE, errno, "%s", infiles[i]);
}

int
main (int argc, char **argv)
{
  int c;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
  hard_LC_COLLATE = hard_locale (LC_COLLATE);

  atexit (close_stdout);

  only_file_1 = true;
  only_file_2 = true;
  both = true;

  while ((c = getopt_long (argc, argv, "123", long_options, NULL)) != -1)
    switch (c)
      {
      case '1':
	only_file_1 = false;
	break;

      case '2':
	only_file_2 = false;
	break;

      case '3':
	both = false;
	break;

      case_GETOPT_HELP_CHAR;

      case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

      default:
	usage (EXIT_FAILURE);
      }

  if (argc - optind < 2)
    {
      if (argc <= optind)
	error (0, 0, _("missing operand"));
      else
	error (0, 0, _("missing operand after %s"), quote (argv[argc - 1]));
      usage (EXIT_FAILURE);
    }

  if (2 < argc - optind)
    {
      error (0, 0, _("extra operand %s"), quote (argv[optind + 2]));
      usage (EXIT_FAILURE);
    }

  compare_files (argv + optind);

  exit (EXIT_SUCCESS);
}
