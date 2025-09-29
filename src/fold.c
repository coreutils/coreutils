/* fold -- wrap each input line to fit in specified width.
   Copyright (C) 1991-2025 Free Software Foundation, Inc.

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

/* Written by David MacKenzie, djm@gnu.ai.mit.edu. */

#include <config.h>

#include <ctype.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "fadvise.h"
#include "ioblksize.h"
#include "mcel.h"
#include "mbbuf.h"
#include "xdectoint.h"

#define TAB_WIDTH 8

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "fold"

#define AUTHORS proper_name ("David MacKenzie")

/* If nonzero, try to break on whitespace. */
static bool break_spaces;

/* Mode to operate in.  */
static enum
  {
    COUNT_COLUMNS,
    COUNT_BYTES,
    COUNT_CHARACTERS
  } counting_mode = COUNT_COLUMNS;

/* If nonzero, at least one of the files we read was standard input. */
static bool have_read_stdin;

/* Width of last read character.  */
static int last_character_width = 0;

static char const shortopts[] = "bcsw:0::1::2::3::4::5::6::7::8::9::";

static struct option const longopts[] =
{
  {"bytes", no_argument, nullptr, 'b'},
  {"characters", no_argument, nullptr, 'c'},
  {"spaces", no_argument, nullptr, 's'},
  {"width", required_argument, nullptr, 'w'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE]...\n\
"),
              program_name);
      fputs (_("\
Wrap input lines in each FILE, writing to standard output.\n\
"), stdout);

      emit_stdin_note ();
      emit_mandatory_arg_note ();

      fputs (_("\
  -b, --bytes         count bytes rather than columns\n\
  -c, --characters    count characters rather than columns\n\
  -s, --spaces        break at spaces\n\
  -w, --width=WIDTH   use WIDTH columns instead of 80\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* Assuming the current column is COLUMN, return the column that
   printing C will move the cursor to.
   The first column is 0. */

static size_t
adjust_column (size_t column, mcel_t g)
{
  if (counting_mode != COUNT_BYTES)
    {
      if (g.ch == '\b')
        {
          if (column > 0)
            column -= last_character_width;
        }
      else if (g.ch == '\r')
        column = 0;
      else if (g.ch == '\t')
        column += TAB_WIDTH - column % TAB_WIDTH;
      else
        {
          if (counting_mode == COUNT_CHARACTERS)
            last_character_width = 1;
          else
            {
              int width = c32width (g.ch);
              /* Default to a width of 1 if there is an invalid character.  */
              last_character_width = width < 0 ? 1 : width;
            }
          column += last_character_width;
        }
    }
  else
    column += g.len;
  return column;
}

static void
write_out (char const *line, size_t line_len, bool newline)
{
  if (fwrite (line, sizeof (char), line_len, stdout) != line_len
      || (newline && putchar ('\n') < 0))
    write_error ();
}

/* Fold file FILENAME, or standard input if FILENAME is "-",
   to stdout, with maximum line length WIDTH.
   Return true if successful.  */

static bool
fold_file (char const *filename, size_t width)
{
  FILE *istream;
  size_t column = 0;		/* Screen column where next char will go. */
  idx_t offset_out = 0;		/* Index in 'line_out' for next char. */
  static char line_out[IO_BUFSIZE];
  static char line_in[IO_BUFSIZE];
  mbbuf_t mbbuf;
  int saved_errno;

  if (streq (filename, "-"))
    {
      istream = stdin;
      have_read_stdin = true;
    }
  else
    istream = fopen (filename, "r");

  if (istream == nullptr)
    {
      error (0, errno, "%s", quotef (filename));
      return false;
    }

  fadvise (istream, FADVISE_SEQUENTIAL);
  mbbuf_init (&mbbuf, line_in, sizeof line_in, istream);

  mcel_t g;
  while ((g = mbbuf_get_char (&mbbuf)).ch != MBBUF_EOF)
    {
      if (g.ch == '\n')
        {
          write_out (line_out, offset_out, /*newline=*/ true);
          column = offset_out = 0;
          continue;
        }
    rescan:
      column = adjust_column (column, g);

      if (column > width)
        {
          /* This character would make the line too long.
             Print the line plus a newline, and make this character
             start the next line. */
          if (break_spaces)
            {
              int space_length = 0;
              idx_t logical_end = offset_out;
              char *logical_p = line_out;
              char *logical_lim = logical_p + logical_end;

              for (mcel_t g2; logical_p < logical_lim; logical_p += g2.len)
                {
                  g2 = mcel_scan (logical_p, logical_lim);
                  if (c32isblank (g2.ch) && ! c32isnbspace (g2.ch))
                    {
                      space_length = g2.len;
                      logical_end = logical_p - line_out;
                    }
                }

              if (space_length)
                {
                  logical_end += space_length;
                  /* Found a blank.  Don't output the part after it. */
                  write_out (line_out, logical_end, /*newline=*/ true);
                  /* Move the remainder to the beginning of the next line.
                     The areas being copied here might overlap. */
                  memmove (line_out, line_out + logical_end,
                           offset_out - logical_end);
                  offset_out -= logical_end;
                  column = 0;
                  char *printed_p = line_out;
                  char *printed_lim = printed_p + offset_out;
                  for (mcel_t g2; printed_p < printed_lim;
                       printed_p += g2.len)
                    {
                      g2 = mcel_scan (printed_p, printed_lim);
                      column = adjust_column (column, g2);
                    }
                  goto rescan;
                }
            }

          if (offset_out == 0)
            {
              memcpy (line_out, mbbuf_char_offset (&mbbuf, g), g.len);
              offset_out += g.len;
              continue;
            }

          write_out (line_out, offset_out, /*newline=*/ true);
          column = offset_out = 0;
          goto rescan;
        }

      /* This can occur if we have read characters with a width of
         zero.  */
      if (sizeof line_out <= offset_out + g.len)
        {
          write_out (line_out, offset_out, /*newline=*/ false);
          offset_out = 0;
        }

      memcpy (line_out + offset_out, mbbuf_char_offset (&mbbuf, g), g.len);
      offset_out += g.len;
    }

  saved_errno = errno;
  if (!ferror (istream))
    saved_errno = 0;

  if (offset_out)
    write_out (line_out, offset_out, /*newline=*/ false);

  if (streq (filename, "-"))
    clearerr (istream);
  else if (fclose (istream) != 0 && !saved_errno)
    saved_errno = errno;

  if (saved_errno)
    {
      error (0, saved_errno, "%s", quotef (filename));
      return false;
    }

  return true;
}

int
main (int argc, char **argv)
{
  size_t width = 80;
  int i;
  int optc;
  bool ok;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  break_spaces = have_read_stdin = false;

  while ((optc = getopt_long (argc, argv, shortopts, longopts, nullptr)) != -1)
    {
      char optargbuf[2];

      switch (optc)
        {
        case 'b':		/* Count bytes rather than columns. */
          counting_mode = COUNT_BYTES;
          break;

        case 'c':               /* Count characters rather than columns. */
          counting_mode = COUNT_CHARACTERS;
          break;

        case 's':		/* Break at word boundaries. */
          break_spaces = true;
          break;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
          if (optarg)
            optarg--;
          else
            {
              optargbuf[0] = optc;
              optargbuf[1] = '\0';
              optarg = optargbuf;
            }
          FALLTHROUGH;
        case 'w':		/* Line width. */
          width = xnumtoumax (optarg, 10, 1, SIZE_MAX - TAB_WIDTH - 1, "",
                              _("invalid number of columns"), 0,
                              XTOINT_MIN_RANGE | XTOINT_MAX_RANGE);
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }
    }

  if (argc == optind)
    ok = fold_file ("-", width);
  else
    {
      ok = true;
      for (i = optind; i < argc; i++)
        ok &= fold_file (argv[i], width);
    }

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, "-");

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
