/* fold -- wrap each input line to fit in specified width.
   Copyright (C) 91, 1995-2002 Free Software Foundation, Inc.

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

/* Written by David MacKenzie, djm@gnu.ai.mit.edu. */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "closeout.h"
#include "error.h"
#include "posixver.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "fold"

#define AUTHORS "David MacKenzie"

/* The name this program was run with. */
char *program_name;

/* If nonzero, try to break on whitespace. */
static int break_spaces;

/* If nonzero, count bytes, not column positions. */
static int count_bytes;

/* If nonzero, at least one of the files we read was standard input. */
static int have_read_stdin;

static struct option const longopts[] =
{
  {"bytes", no_argument, NULL, 'b'},
  {"spaces", no_argument, NULL, 's'},
  {"width", required_argument, NULL, 'w'},
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
      printf (_("\
Usage: %s [OPTION]... [FILE]...\n\
"),
	      program_name);
      fputs (_("\
Wrap input lines in each FILE (standard input by default), writing to\n\
standard output.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -b, --bytes         count bytes rather than columns\n\
  -s, --spaces        break at spaces\n\
  -w, --width=WIDTH   use WIDTH columns instead of 80\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Assuming the current column is COLUMN, return the column that
   printing C will move the cursor to.
   The first column is 0. */

static int
adjust_column (int column, char c)
{
  if (!count_bytes)
    {
      if (c == '\b')
	{
	  if (column > 0)
	    column--;
	}
      else if (c == '\r')
	column = 0;
      else if (c == '\t')
	column = column + 8 - column % 8;
      else /* if (isprint (c)) */
	column++;
    }
  else
    column++;
  return column;
}

/* Fold file FILENAME, or standard input if FILENAME is "-",
   to stdout, with maximum line length WIDTH.
   Return 0 if successful, 1 if an error occurs. */

static int
fold_file (char *filename, int width)
{
  FILE *istream;
  register int c;
  int column = 0;		/* Screen column where next char will go. */
  int offset_out = 0;	/* Index in `line_out' for next char. */
  static char *line_out = NULL;
  static int allocated_out = 0;

  if (STREQ (filename, "-"))
    {
      istream = stdin;
      have_read_stdin = 1;
    }
  else
    istream = fopen (filename, "r");

  if (istream == NULL)
    {
      error (0, errno, "%s", filename);
      return 1;
    }

  while ((c = getc (istream)) != EOF)
    {
      if (offset_out + 1 >= allocated_out)
	{
	  allocated_out += 1024;
	  line_out = xrealloc (line_out, allocated_out);
	}

      if (c == '\n')
	{
	  line_out[offset_out++] = c;
	  fwrite (line_out, sizeof (char), (size_t) offset_out, stdout);
	  column = offset_out = 0;
	  continue;
	}

    rescan:
      column = adjust_column (column, c);

      if (column > width)
	{
	  /* This character would make the line too long.
	     Print the line plus a newline, and make this character
	     start the next line. */
	  if (break_spaces)
	    {
	      /* Look for the last blank. */
	      int logical_end;

	      for (logical_end = offset_out - 1; logical_end >= 0;
		   logical_end--)
		if (ISBLANK (line_out[logical_end]))
		  break;
	      if (logical_end >= 0)
		{
		  int i;

		  /* Found a blank.  Don't output the part after it. */
		  logical_end++;
		  fwrite (line_out, sizeof (char), (size_t) logical_end,
			  stdout);
		  putchar ('\n');
		  /* Move the remainder to the beginning of the next line.
		     The areas being copied here might overlap. */
		  memmove (line_out, line_out + logical_end,
			   offset_out - logical_end);
		  offset_out -= logical_end;
		  for (column = i = 0; i < offset_out; i++)
		    column = adjust_column (column, line_out[i]);
		  goto rescan;
		}
	    }
	  else
	    {
	      if (offset_out == 0)
		{
		  line_out[offset_out++] = c;
		  continue;
		}
	    }
	  line_out[offset_out++] = '\n';
	  fwrite (line_out, sizeof (char), (size_t) offset_out, stdout);
	  column = offset_out = 0;
	  goto rescan;
	}

      line_out[offset_out++] = c;
    }

  if (offset_out)
    fwrite (line_out, sizeof (char), (size_t) offset_out, stdout);

  if (ferror (istream))
    {
      error (0, errno, "%s", filename);
      if (!STREQ (filename, "-"))
	fclose (istream);
      return 1;
    }
  if (!STREQ (filename, "-") && fclose (istream) == EOF)
    {
      error (0, errno, "%s", filename);
      return 1;
    }

  return 0;
}

int
main (int argc, char **argv)
{
  int width = 80;
  int i;
  int optc;
  int errs = 0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  break_spaces = count_bytes = have_read_stdin = 0;

  /* Turn any numeric options into -w options.  */
  for (i = 1; i < argc; i++)
    {
      char const *a = argv[i];
      if (a[0] == '-')
	{
	  if (a[1] == '-' && ! a[2])
	    break;
	  if (ISDIGIT (a[1]))
	    {
	      char *s = xmalloc (strlen (a) + 2);
	      s[0] = '-';
	      s[1] = 'w';
	      strcpy (s + 2, a + 1);
	      argv[i] = s;
	      if (200112 <= posix2_version ())
		{
		  error (0, 0, _("`%s' option is obsolete; use `%s'"), a, s);
		  usage (EXIT_FAILURE);
		}
	    }
	}
    }

  while ((optc = getopt_long (argc, argv, "bsw:", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;

	case 'b':		/* Count bytes rather than columns. */
	  count_bytes = 1;
	  break;

	case 's':		/* Break at word boundaries. */
	  break_spaces = 1;
	  break;

	case 'w':		/* Line width. */
	  {
	    long int tmp_long;
	    if (xstrtol (optarg, NULL, 10, &tmp_long, "") != LONGINT_OK
		|| tmp_long <= 0 || tmp_long > INT_MAX)
	      error (EXIT_FAILURE, 0,
		     _("invalid number of columns: `%s'"), optarg);
	    width = (int) tmp_long;
	  }
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (argc == optind)
    errs |= fold_file ("-", width);
  else
    for (i = optind; i < argc; i++)
      errs |= fold_file (argv[i], width);

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, "-");

  exit (errs == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
