/* expand - convert tabs to spaces
   Copyright (C) 89, 91, 1995-2002 Free Software Foundation, Inc.

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

/* By default, convert all tabs to spaces.
   Preserves backspace characters in the output; they decrement the
   column count for tab calculations.
   The default action is equivalent to -8.

   Options:
   --tabs=tab1[,tab2[,...]]
   -t tab1[,tab2[,...]]
   -tab1[,tab2[,...]]	If only one tab stop is given, set the tabs tab1
			spaces apart instead of the default 8.  Otherwise,
			set the tabs at columns tab1, tab2, etc. (numbered from
			0); replace any tabs beyond the tabstops given with
			single spaces.
   --initial
   -i			Only convert initial tabs on each line to spaces.

   David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include "system.h"
#include "closeout.h"
#include "error.h"
#include "posixver.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "expand"

#define AUTHORS "David MacKenzie"

/* The number of bytes added at a time to the amount of memory
   allocated for the output line. */
#define OUTPUT_BLOCK 256

/* The number of bytes added at a time to the amount of memory
   allocated for the list of tabstops. */
#define TABLIST_BLOCK 256

/* The name this program was run with. */
char *program_name;

/* If nonzero, convert blanks even after nonblank characters have been
   read on the line. */
static int convert_entire_line;

/* If nonzero, the size of all tab stops.  If zero, use `tab_list' instead. */
static int tab_size;

/* Array of the explicit column numbers of the tab stops;
   after `tab_list' is exhausted, each additional tab is replaced
   by a space.  The first column is column 0. */
static int *tab_list;

/* The index of the first invalid element of `tab_list',
   where the next element can be added. */
static int first_free_tab;

/* Null-terminated array of input filenames. */
static char **file_list;

/* Default for `file_list' if no files are given on the command line. */
static char *stdin_argv[] =
{
  "-", NULL
};

/* Nonzero if we have ever read standard input. */
static int have_read_stdin;

/* Status to return to the system. */
static int exit_status;

static struct option const longopts[] =
{
  {"tabs", required_argument, NULL, 't'},
  {"initial", no_argument, NULL, 'i'},
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
Convert tabs in each FILE to spaces, writing to standard output.\n\
With no FILE, or when FILE is -, read standard input.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -i, --initial       do not convert TABs after non whitespace\n\
  -t, --tabs=NUMBER   have tabs NUMBER characters apart, not 8\n\
"), stdout);
      fputs (_("\
  -t, --tabs=LIST     use comma separated list of explicit tab positions\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Add tab stop TABVAL to the end of `tab_list', except
   if TABVAL is -1, do nothing. */

static void
add_tabstop (int tabval)
{
  if (tabval == -1)
    return;
  if (first_free_tab % TABLIST_BLOCK == 0)
    tab_list = (int *) xrealloc ((char *) tab_list,
				 (first_free_tab
				  + TABLIST_BLOCK * sizeof (tab_list[0])));
  tab_list[first_free_tab++] = tabval;
}

/* Add the comma or blank separated list of tabstops STOPS
   to the list of tabstops. */

static void
parse_tabstops (char *stops)
{
  int tabval = -1;

  for (; *stops; stops++)
    {
      if (*stops == ',' || ISBLANK (*stops))
	{
	  add_tabstop (tabval);
	  tabval = -1;
	}
      else if (ISDIGIT (*stops))
	{
	  if (tabval == -1)
	    tabval = 0;
	  tabval = tabval * 10 + *stops - '0';
	}
      else
	error (EXIT_FAILURE, 0, _("tab size contains an invalid character"));
    }

  add_tabstop (tabval);
}

/* Check that the list of tabstops TABS, with ENTRIES entries,
   contains only nonzero, ascending values. */

static void
validate_tabstops (int *tabs, int entries)
{
  int prev_tab = 0;
  int i;

  for (i = 0; i < entries; i++)
    {
      if (tabs[i] == 0)
	error (EXIT_FAILURE, 0, _("tab size cannot be 0"));
      if (tabs[i] <= prev_tab)
	error (EXIT_FAILURE, 0, _("tab sizes must be ascending"));
      prev_tab = tabs[i];
    }
}

/* Close the old stream pointer FP if it is non-NULL,
   and return a new one opened to read the next input file.
   Open a filename of `-' as the standard input.
   Return NULL if there are no more input files.  */

static FILE *
next_file (FILE *fp)
{
  static char *prev_file;
  char *file;

  if (fp)
    {
      if (ferror (fp))
	{
	  error (0, errno, "%s", prev_file);
	  exit_status = 1;
	}
      if (fp == stdin)
	clearerr (fp);		/* Also clear EOF. */
      else if (fclose (fp) == EOF)
	{
	  error (0, errno, "%s", prev_file);
	  exit_status = 1;
	}
    }

  while ((file = *file_list++) != NULL)
    {
      if (file[0] == '-' && file[1] == '\0')
	{
	  have_read_stdin = 1;
	  prev_file = file;
	  return stdin;
	}
      fp = fopen (file, "r");
      if (fp)
	{
	  prev_file = file;
	  return fp;
	}
      error (0, errno, "%s", file);
      exit_status = 1;
    }
  return NULL;
}

/* Change tabs to spaces, writing to stdout.
   Read each file in `file_list', in order. */

static void
expand (void)
{
  FILE *fp;			/* Input stream. */
  int c;			/* Each input character. */
  int tab_index = 0;		/* Index in `tab_list' of next tabstop. */
  int column = 0;		/* Column on screen of the next char. */
  int next_tab_column;		/* Column the next tab stop is on. */
  int convert = 1;		/* If nonzero, perform translations. */

  fp = next_file ((FILE *) NULL);
  if (fp == NULL)
    return;

  /* Binary I/O will preserve the original EOL style (DOS/Unix) of files.  */
  SET_BINARY2 (fileno (fp), STDOUT_FILENO);

  for (;;)
    {
      c = getc (fp);
      if (c == EOF)
	{
	  fp = next_file (fp);
	  if (fp == NULL)
	    break;		/* No more files. */
	  else
	    {
	      SET_BINARY2 (fileno (fp), STDOUT_FILENO);
	      continue;
	    }
	}

      if (c == '\n')
	{
	  putchar (c);
	  tab_index = 0;
	  column = 0;
	  convert = 1;
	}
      else if (c == '\t' && convert)
	{
	  if (tab_size == 0)
	    {
	      /* Do not let tab_index == first_free_tab;
		 stop when it is 1 less. */
	      while (tab_index < first_free_tab - 1
		     && column >= tab_list[tab_index])
		tab_index++;
	      next_tab_column = tab_list[tab_index];
	      if (tab_index < first_free_tab - 1)
		tab_index++;
	      if (column >= next_tab_column)
		next_tab_column = column + 1; /* Ran out of tab stops. */
	    }
	  else
	    {
	      next_tab_column = column + tab_size - column % tab_size;
	    }
	  while (column < next_tab_column)
	    {
	      putchar (' ');
	      ++column;
	    }
	}
      else
	{
	  if (convert)
	    {
	      if (c == '\b')
		{
		  if (column > 0)
		    --column;
		}
	      else
		{
		  ++column;
		  if (convert_entire_line == 0)
		    convert = 0;
		}
	    }
	  putchar (c);
	}
    }
}

int
main (int argc, char **argv)
{
  int tabval = -1;		/* Value of tabstop being read, or -1. */
  int c;			/* Option character. */

  bool obsolete_tablist = false;

  have_read_stdin = 0;
  exit_status = 0;
  convert_entire_line = 1;
  tab_list = NULL;
  first_free_tab = 0;
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((c = getopt_long (argc, argv, "it:,0123456789", longopts, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case '?':
	  usage (EXIT_FAILURE);
	case 'i':
	  convert_entire_line = 0;
	  break;
	case 't':
	  parse_tabstops (optarg);
	  break;
	case ',':
	  add_tabstop (tabval);
	  tabval = -1;
	  obsolete_tablist = true;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  if (tabval == -1)
	    tabval = 0;
	  tabval = tabval * 10 + c - '0';
	  obsolete_tablist = true;
	  break;
	}
    }

  if (obsolete_tablist && 200112 <= posix2_version ())
    {
      error (0, 0, _("`-LIST' option is obsolete; use `-t LIST'"));
      usage (EXIT_FAILURE);
    }

  add_tabstop (tabval);

  validate_tabstops (tab_list, first_free_tab);

  if (first_free_tab == 0)
    tab_size = 8;
  else if (first_free_tab == 1)
    tab_size = tab_list[0];
  else
    tab_size = 0;

  if (optind == argc)
    file_list = stdin_argv;
  else
    file_list = &argv[optind];

  expand ();

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, "-");

  exit (exit_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
