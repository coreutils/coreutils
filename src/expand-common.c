/* expand-common - common functionality for expand/unexpand
   Copyright (C) 1989-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include "c-ctype.h"
#include "fadvise.h"
#include "quote.h"

#include "expand-common.h"

/* If true, convert blanks even after nonblank characters have been
   read on the line.  */
bool convert_entire_line = false;

/* If nonzero, the size of all tab stops.  If zero, use 'tab_list' instead.  */
static colno tab_size = 0;

/* If nonzero, the size of all tab stops after the last specified.  */
static colno extend_size = 0;

/* If nonzero, an increment for additional tab stops after the last specified.*/
static colno increment_size = 0;

/* The maximum distance between tab stops.  */
idx_t max_column_width;

/* Array of the explicit column numbers of the tab stops;
   after 'tab_list' is exhausted, each additional tab is replaced
   by a space.  The first column is column 0.  */
static colno *tab_list = nullptr;

/* The number of allocated entries in 'tab_list'.  */
static idx_t n_tabs_allocated = 0;

/* The index of the first invalid element of 'tab_list',
   where the next element can be added.  */
static idx_t first_free_tab = 0;

/* Null-terminated array of input filenames.  */
static char **file_list = nullptr;

/* Default for 'file_list' if no files are given on the command line.  */
static char *stdin_argv[] =
{
  (char *) "-", nullptr
};

/* True if we have ever read standard input.  */
static bool have_read_stdin = false;

/* The desired exit status.  */
int exit_status = EXIT_SUCCESS;


static void
set_max_column_width (colno width)
{
  if (max_column_width < width)
    {
      if (ckd_add (&max_column_width, width, 0))
        error (EXIT_FAILURE, 0, _("tabs are too far apart"));
    }
}

/* Add tab stop TABVAL to the end of 'tab_list'.  */
extern void
add_tab_stop (colno tabval)
{
  colno prev_column = first_free_tab ? tab_list[first_free_tab - 1] : 0;
  colno column_width = prev_column <= tabval ? tabval - prev_column : 0;

  if (first_free_tab == n_tabs_allocated)
    tab_list = xpalloc (tab_list, &n_tabs_allocated, 1, -1, sizeof *tab_list);
  tab_list[first_free_tab++] = tabval;

  set_max_column_width (column_width);
}

static bool
set_extend_size (colno tabval)
{
  bool ok = true;

  if (extend_size)
    {
      error (0, 0,
             _("'/' specifier only allowed"
               " with the last value"));
      ok = false;
    }
  extend_size = tabval;

  set_max_column_width (extend_size);

  return ok;
}

static bool
set_increment_size (colno tabval)
{
  bool ok = true;

  if (increment_size)
    {
      error (0,0,
             _("'+' specifier only allowed"
               " with the last value"));
      ok = false;
    }
  increment_size = tabval;

  set_max_column_width (increment_size);

  return ok;
}

/* Add the comma or blank separated list of tab stops STOPS
   to the list of tab stops.  */
extern void
parse_tab_stops (char const *stops)
{
  bool have_tabval = false;
  colno tabval = 0;
  bool extend_tabval = false;
  bool increment_tabval = false;
  char const *num_start = nullptr;
  bool ok = true;

  for (; *stops; stops++)
    {
      if (*stops == ',' || isblank (to_uchar (*stops)))
        {
          if (have_tabval)
            {
              if (extend_tabval)
                {
                  if (! set_extend_size (tabval))
                    {
                      ok = false;
                      break;
                    }
                }
              else if (increment_tabval)
                {
                  if (! set_increment_size (tabval))
                    {
                      ok = false;
                      break;
                    }
                }
              else
                add_tab_stop (tabval);
            }
          have_tabval = false;
        }
      else if (*stops == '/')
        {
          if (have_tabval)
            {
              error (0, 0, _("'/' specifier not at start of number: %s"),
                     quote (stops));
              ok = false;
            }
          extend_tabval = true;
          increment_tabval = false;
        }
      else if (*stops == '+')
        {
          if (have_tabval)
            {
              error (0, 0, _("'+' specifier not at start of number: %s"),
                     quote (stops));
              ok = false;
            }
          increment_tabval = true;
          extend_tabval = false;
        }
      else if (c_isdigit (*stops))
        {
          if (!have_tabval)
            {
              tabval = 0;
              have_tabval = true;
              num_start = stops;
            }

          /* Detect overflow.  */
          if (!DECIMAL_DIGIT_ACCUMULATE (tabval, *stops - '0'))
            {
              idx_t len = strspn (num_start, "0123456789");
              char *bad_num = ximemdup0 (num_start, len);
              error (0, 0, _("tab stop is too large %s"), quote (bad_num));
              free (bad_num);
              ok = false;
              stops = num_start + len - 1;
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

  if (ok && have_tabval)
    {
      if (extend_tabval)
        ok &= set_extend_size (tabval);
      else if (increment_tabval)
        ok &= set_increment_size (tabval);
      else
        add_tab_stop (tabval);
    }

  if (! ok)
    exit (EXIT_FAILURE);
}

/* Check that the list of tab stops TABS, with ENTRIES entries,
   contains only nonzero, ascending values.  */

static void
validate_tab_stops (colno const *tabs, idx_t entries)
{
  colno prev_tab = 0;

  for (idx_t i = 0; i < entries; i++)
    {
      if (tabs[i] == 0)
        error (EXIT_FAILURE, 0, _("tab size cannot be 0"));
      if (tabs[i] <= prev_tab)
        error (EXIT_FAILURE, 0, _("tab sizes must be ascending"));
      prev_tab = tabs[i];
    }

  if (increment_size && extend_size)
    error (EXIT_FAILURE, 0, _("'/' specifier is mutually exclusive with '+'"));
}

/* Called after all command-line options have been parsed,
   and add_tab_stop/parse_tab_stops have been called.
   Will validate the tab-stop values,
   and set the final values to:
   tab-stops = 8 (if no tab-stops given on command line)
   tab-stops = N (if value N specified as the only value).
   tab-stops = distinct values given on command line (if multiple values given).
*/
extern void
finalize_tab_stops (void)
{
  validate_tab_stops (tab_list, first_free_tab);

  if (first_free_tab == 0)
    tab_size = max_column_width = extend_size
                                  ? extend_size : increment_size
                                                  ? increment_size : 8;
  else if (first_free_tab == 1 && ! extend_size && ! increment_size)
    tab_size = tab_list[0];
  else
    tab_size = 0;
}


/* Return number of first tab stop after COLUMN.  TAB_INDEX specifies
   many multiple tab-sizes.  Set *LAST_TAB depending on whether we are
   returning COLUMN + 1 merely because we're past the last tab.
   If the number would overflow, diagnose this and exit.  */
extern colno
get_next_tab_column (colno column, idx_t *tab_index, bool *last_tab)
{
  *last_tab = false;
  colno tab_distance;

  /* single tab-size - return multiples of it */
  if (tab_size)
    tab_distance = tab_size - column % tab_size;
  else
    {
      /* multiple tab-sizes - iterate them until the tab position is beyond
         the current input column. */
      for ( ; *tab_index < first_free_tab ; (*tab_index)++ )
        {
          colno tab = tab_list[*tab_index];
          if (column < tab)
            return tab;
        }

      /* relative last tab - return multiples of it */
      if (extend_size)
        tab_distance = extend_size - column % extend_size;
      else if (increment_size)
        {
          /* incremental last tab - add increment_size to the previous
             tab stop */
          colno end_tab = tab_list[first_free_tab - 1];
          tab_distance = increment_size - ((column - end_tab) % increment_size);
        }
      else
        {
          *last_tab = true;
          tab_distance = 1;
        }
    }

  colno tab_stop;
  if (ckd_add (&tab_stop, column, tab_distance))
    error (EXIT_FAILURE, 0, _("input line is too long"));
  return tab_stop;
}


/* Sets new file-list */
extern void
set_file_list (char **list)
{
  have_read_stdin = false;

  if (!list)
    file_list = stdin_argv;
  else
    file_list = list;
}

/* Close the old stream pointer FP if it is non-null,
   and return a new one opened to read the next input file.
   Open a filename of '-' as the standard input.
   Return nullptr if there are no more input files.  */

extern FILE *
next_file (FILE *fp)
{
  static char *prev_file;
  char *file;

  if (fp)
    {
      int err = errno;
      if (!ferror (fp))
        err = 0;
      if (streq (prev_file, "-"))
        clearerr (fp);		/* Also clear EOF.  */
      else if (fclose (fp) != 0)
        err = errno;
      if (err)
        {
          error (0, err, "%s", quotef (prev_file));
          exit_status = EXIT_FAILURE;
        }
    }

  while ((file = *file_list++) != nullptr)
    {
      if (streq (file, "-"))
        {
          have_read_stdin = true;
          fp = stdin;
        }
      else
        fp = fopen (file, "r");
      if (fp)
        {
          prev_file = file;
          fadvise (fp, FADVISE_SEQUENTIAL);
          return fp;
        }
      error (0, errno, "%s", quotef (file));
      exit_status = EXIT_FAILURE;
    }
  return nullptr;
}

/* Close standard input if we have read from it.  */
extern void
cleanup_file_list_stdin (void)
{
    if (have_read_stdin && fclose (stdin) != 0)
      error (EXIT_FAILURE, errno, "-");
}

/* Emit the --help output for --tabs=LIST option accepted by expand and
   unexpand.  */
extern void
emit_tab_list_info (void)
{
  /* suppress syntax check for emit_mandatory_arg_note() */
  fputs (_("\
  -t, --tabs=LIST  use comma separated list of tab positions.\n\
"), stdout);
  fputs (_("\
                     The last specified position can be prefixed with '/'\n\
                     to specify a tab size to use after the last\n\
                     explicitly specified tab stop.  Also a prefix of '+'\n\
                     can be used to align remaining tab stops relative to\n\
                     the last specified tab stop instead of the first column\n\
"), stdout);
}
