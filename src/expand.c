/* expand - convert tabs to spaces
   Copyright (C) 89, 91, 1995-2004 Free Software Foundation, Inc.

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
#include "error.h"
#include "posixver.h"
#include "quote.h"
#include "xstrndup.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "expand"

#define AUTHORS "David MacKenzie"

/* The number of bytes added at a time to the amount of memory
   allocated for the output line.  */
#define OUTPUT_BLOCK 256

/* The name this program was run with.  */
char *program_name;

/* If true, convert blanks even after nonblank characters have been
   read on the line.  */
static bool convert_entire_line;

/* If nonzero, the size of all tab stops.  If zero, use `tab_list' instead.  */
static uintmax_t tab_size;

/* Array of the explicit column numbers of the tab stops;
   after `tab_list' is exhausted, each additional tab is replaced
   by a space.  The first column is column 0.  */
static uintmax_t *tab_list;

/* The number of allocated entries in `tab_list'.  */
static size_t n_tabs_allocated;

/* The index of the first invalid element of `tab_list',
   where the next element can be added.  */
static size_t first_free_tab;

/* Null-terminated array of input filenames.  */
static char **file_list;

/* Default for `file_list' if no files are given on the command line.  */
static char *stdin_argv[] =
{
  "-", NULL
};

/* True if we have ever read standard input.  */
static bool have_read_stdin;

/* The desired exit status.  */
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
  if (status != EXIT_SUCCESS)
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
  exit (status);
}

/* Add tab stop TABVAL to the end of `tab_list'.  */

static void
add_tabstop (uintmax_t tabval)
{
  if (first_free_tab == n_tabs_allocated)
    tab_list = x2nrealloc (tab_list, &n_tabs_allocated, sizeof *tab_list);
  tab_list[first_free_tab++] = tabval;
}

/* Add the comma or blank separated list of tabstops STOPS
   to the list of tabstops.  */

static void
parse_tabstops (char const *stops)
{
  bool have_tabval = false;
  uintmax_t tabval IF_LINT (= 0);
  char const *num_start IF_LINT (= NULL);
  bool ok = true;

  for (; *stops; stops++)
    {
      if (*stops == ',' || ISBLANK (to_uchar (*stops)))
	{
	  if (have_tabval)
	    add_tabstop (tabval);
	  have_tabval = false;
	}
      else if (ISDIGIT (*stops))
	{
	  if (!have_tabval)
	    {
	      tabval = 0;
	      have_tabval = true;
	      num_start = stops;
	    }
	  {
	    /* Detect overflow.  */
	    uintmax_t new_t = 10 * tabval + *stops - '0';
	    if (UINTMAX_MAX / 10 < tabval || new_t < tabval * 10)
	      {
		size_t len = strspn (num_start, "0123456789");
		char *bad_num = xstrndup (num_start, len);
		error (0, 0, _("tab stop is too large %s"), quote (bad_num));
		free (bad_num);
		ok = false;
		stops = num_start + len - 1;
	      }
	    tabval = new_t;
	  }
	}
      else
	{
	  error (0, 0, _("tab size contains invalid character(s): %s"),
		 quote (stops));
	  ok = false;
	  break;
	}
    }

  if (!ok)
    exit (EXIT_FAILURE);

  if (have_tabval)
    add_tabstop (tabval);
}

/* Check that the list of tabstops TABS, with ENTRIES entries,
   contains only nonzero, ascending values.  */

static void
validate_tabstops (uintmax_t const *tabs, size_t entries)
{
  uintmax_t prev_tab = 0;
  size_t i;

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
	  exit_status = EXIT_FAILURE;
	}
      if (fp == stdin)
	clearerr (fp);		/* Also clear EOF.  */
      else if (fclose (fp) == EOF)
	{
	  error (0, errno, "%s", prev_file);
	  exit_status = EXIT_FAILURE;
	}
    }

  while ((file = *file_list++) != NULL)
    {
      if (file[0] == '-' && file[1] == '\0')
	{
	  have_read_stdin = true;
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
      exit_status = EXIT_FAILURE;
    }
  return NULL;
}

/* Change tabs to spaces, writing to stdout.
   Read each file in `file_list', in order.  */

static void
expand (void)
{
  FILE *fp;			/* Input stream.  */
  size_t tab_index = 0;		/* Index in `tab_list' of next tabstop.  */
  uintmax_t column = 0;		/* Column of next char.  */
  uintmax_t next_tab_column;	/* Column the next tab stop is on.  */
  bool convert = true;		/* If true, perform translations.  */

  fp = next_file ((FILE *) NULL);
  if (fp == NULL)
    return;

  /* Binary I/O will preserve the original EOL style (DOS/Unix) of files.  */
  SET_BINARY2 (fileno (fp), STDOUT_FILENO);

  for (;;)
    {
      int c = getc (fp);
      if (c == EOF)
	{
	  fp = next_file (fp);
	  if (fp)
	    {
	      SET_BINARY2 (fileno (fp), STDOUT_FILENO);
	      continue;
	    }
	  break;
	}

      if (c == '\n')
	{
	  putchar (c);
	  tab_index = 0;
	  column = 0;
	  convert = true;
	}
      else if (c == '\t' && convert)
	{
	  if (tab_size == 0)
	    {
	      /* Do not let tab_index == first_free_tab;
		 stop when it is 1 less.  */
	      while (tab_index < first_free_tab - 1
		     && column >= tab_list[tab_index])
		tab_index++;
	      next_tab_column = tab_list[tab_index];
	      if (tab_index < first_free_tab - 1)
		tab_index++;
	      if (column >= next_tab_column)
		next_tab_column = column + 1; /* Ran out of tab stops.  */
	    }
	  else
	    {
	      next_tab_column = column + tab_size - column % tab_size;
	    }
	  if (next_tab_column < column)
	    error (EXIT_FAILURE, 0, _("input line is too long"));
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
		    {
		      column--;
		      tab_index -= (tab_index != 0);
		    }
		}
	      else
		{
		  ++column;
		  if (column == 0)
		    error (EXIT_FAILURE, 0, _("input line is too long"));
		  convert &= convert_entire_line;
		}
	    }
	  putchar (c);
	}
    }
}

int
main (int argc, char **argv)
{
  bool have_tabval = false;
  uintmax_t tabval IF_LINT (= 0);
  int c;

  bool obsolete_tablist = false;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  have_read_stdin = false;
  exit_status = EXIT_SUCCESS;
  convert_entire_line = true;
  tab_list = NULL;
  first_free_tab = 0;

  while ((c = getopt_long (argc, argv, "it:,0123456789", longopts, NULL))
	 != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case '?':
	  usage (EXIT_FAILURE);
	case 'i':
	  convert_entire_line = false;
	  break;
	case 't':
	  parse_tabstops (optarg);
	  break;
	case ',':
	  if (have_tabval)
	    add_tabstop (tabval);
	  have_tabval = false;
	  obsolete_tablist = true;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  if (!have_tabval)
	    {
	      tabval = 0;
	      have_tabval = true;
	    }
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

  if (have_tabval)
    add_tabstop (tabval);

  validate_tabstops (tab_list, first_free_tab);

  if (first_free_tab == 0)
    tab_size = 8;
  else if (first_free_tab == 1)
    tab_size = tab_list[0];
  else
    tab_size = 0;

  file_list = (optind < argc ? &argv[optind] : stdin_argv);

  expand ();

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, "-");

  exit (exit_status);
}
