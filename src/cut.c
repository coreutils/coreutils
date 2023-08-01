/* cut - remove parts of lines of files
   Copyright (C) 1997-2023 Free Software Foundation, Inc.
   Copyright (C) 1984 David M. Ihnat

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

/* Written by David Ihnat.  */

/* POSIX changes, bug fixes, long-named options, and cleanup
   by David MacKenzie <djm@gnu.ai.mit.edu>.

   Rewrite cut_fields and cut_bytes -- Jim Meyering.  */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include "system.h"

#include "assure.h"
#include "fadvise.h"
#include "getndelim2.h"

#include "set-fields.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "cut"

#define AUTHORS \
  proper_name ("David M. Ihnat"), \
  proper_name ("David MacKenzie"), \
  proper_name ("Jim Meyering")

#define FATAL_ERROR(Message)						\
  do									\
    {									\
      error (0, 0, (Message));						\
      usage (EXIT_FAILURE);						\
    }									\
  while (0)


/* Pointer inside RP.  When checking if a byte or field is selected
   by a finite range, we check if it is between CURRENT_RP.LO
   and CURRENT_RP.HI.  If the byte or field index is greater than
   CURRENT_RP.HI then we make CURRENT_RP to point to the next range pair.  */
static struct field_range_pair *current_rp;

/* This buffer is used to support the semantics of the -s option
   (or lack of same) when the specified field list includes (does
   not include) the first field.  In both of those cases, the entire
   first field must be read into this buffer to determine whether it
   is followed by a delimiter or a newline before any of it may be
   output.  Otherwise, cut_fields can do the job without using this
   buffer.  */
static char *field_1_buffer;

/* The number of bytes allocated for FIELD_1_BUFFER.  */
static size_t field_1_bufsize;

/* If true, do not output lines containing no delimiter characters.
   Otherwise, all such lines are printed.  This option is valid only
   with field mode.  */
static bool suppress_non_delimited;

/* If true, print all bytes, characters, or fields _except_
   those that were specified.  */
static bool complement;

/* The delimiter character for field mode.  */
static unsigned char delim;

/* The delimiter for each line/record.  */
static unsigned char line_delim = '\n';

/* The length of output_delimiter_string.  */
static size_t output_delimiter_length;

/* The output field separator string.  Defaults to the 1-character
   string consisting of the input delimiter.  */
static char *output_delimiter_string;

/* The output delimiter string contents, if the default.  */
static char output_delimiter_default[1];

/* True if we have ever read standard input.  */
static bool have_read_stdin;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  OUTPUT_DELIMITER_OPTION = CHAR_MAX + 1,
  COMPLEMENT_OPTION
};

static struct option const longopts[] =
{
  {"bytes", required_argument, nullptr, 'b'},
  {"characters", required_argument, nullptr, 'c'},
  {"fields", required_argument, nullptr, 'f'},
  {"delimiter", required_argument, nullptr, 'd'},
  {"only-delimited", no_argument, nullptr, 's'},
  {"output-delimiter", required_argument, nullptr, OUTPUT_DELIMITER_OPTION},
  {"complement", no_argument, nullptr, COMPLEMENT_OPTION},
  {"zero-terminated", no_argument, nullptr, 'z'},
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
Usage: %s OPTION... [FILE]...\n\
"),
              program_name);
      fputs (_("\
Print selected parts of lines from each FILE to standard output.\n\
"), stdout);

      emit_stdin_note ();
      emit_mandatory_arg_note ();

      fputs (_("\
  -b, --bytes=LIST        select only these bytes\n\
  -c, --characters=LIST   select only these characters\n\
  -d, --delimiter=DELIM   use DELIM instead of TAB for field delimiter\n\
"), stdout);
      fputs (_("\
  -f, --fields=LIST       select only these fields;  also print any line\n\
                            that contains no delimiter character, unless\n\
                            the -s option is specified\n\
  -n                      (ignored)\n\
"), stdout);
      fputs (_("\
      --complement        complement the set of selected bytes, characters\n\
                            or fields\n\
"), stdout);
      fputs (_("\
  -s, --only-delimited    do not print lines not containing delimiters\n\
      --output-delimiter=STRING  use STRING as the output delimiter\n\
                            the default is to use the input delimiter\n\
"), stdout);
      fputs (_("\
  -z, --zero-terminated   line delimiter is NUL, not newline\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
Use one, and only one of -b, -c or -f.  Each LIST is made up of one\n\
range, or many ranges separated by commas.  Selected input is written\n\
in the same order that it is read, and is written exactly once.\n\
"), stdout);
      fputs (_("\
Each range is one of:\n\
\n\
  N     N'th byte, character or field, counted from 1\n\
  N-    from N'th byte, character or field, to end of line\n\
  N-M   from N'th to M'th (included) byte, character or field\n\
  -M    from first to M'th (included) byte, character or field\n\
"), stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}


/* Increment *ITEM_IDX (i.e., a field or byte index),
   and if required CURRENT_RP.  */

static inline void
next_item (uintmax_t *item_idx)
{
  (*item_idx)++;
  if ((*item_idx) > current_rp->hi)
    current_rp++;
}

/* Return nonzero if the K'th field or byte is printable.  */

static inline bool
print_kth (uintmax_t k)
{
  return current_rp->lo <= k;
}

/* Return nonzero if K'th byte is the beginning of a range.  */

static inline bool
is_range_start_index (uintmax_t k)
{
  return k == current_rp->lo;
}

/* Read from stream STREAM, printing to standard output any selected bytes.  */

static void
cut_bytes (FILE *stream)
{
  uintmax_t byte_idx;	/* Number of bytes in the line so far.  */
  /* Whether to begin printing delimiters between ranges for the current line.
     Set after we've begun printing data corresponding to the first range.  */
  bool print_delimiter;

  byte_idx = 0;
  print_delimiter = false;
  current_rp = frp;
  while (true)
    {
      int c;		/* Each character from the file.  */

      c = getc (stream);

      if (c == line_delim)
        {
          if (putchar (c) < 0)
            write_error ();
          byte_idx = 0;
          print_delimiter = false;
          current_rp = frp;
        }
      else if (c == EOF)
        {
          if (byte_idx > 0)
          {
            if (putchar (line_delim) < 0)
              write_error ();
          }
          break;
        }
      else
        {
          next_item (&byte_idx);
          if (print_kth (byte_idx))
            {
              if (output_delimiter_string != output_delimiter_default)
                {
                  if (print_delimiter && is_range_start_index (byte_idx))
                    {
                      if (fwrite (output_delimiter_string, sizeof (char),
                                  output_delimiter_length, stdout)
                          != output_delimiter_length)
                        write_error ();
                    }
                  print_delimiter = true;
                }

              if (putchar (c) < 0)
                write_error ();
            }
        }
    }
}

/* Read from stream STREAM, printing to standard output any selected fields.  */

static void
cut_fields (FILE *stream)
{
  int c;	/* Each character from the file.  */
  uintmax_t field_idx = 1;
  bool found_any_selected_field = false;
  bool buffer_first_field;

  current_rp = frp;

  c = getc (stream);
  if (c == EOF)
    return;

  ungetc (c, stream);
  c = 0;

  /* To support the semantics of the -s flag, we may have to buffer
     all of the first field to determine whether it is 'delimited.'
     But that is unnecessary if all non-delimited lines must be printed
     and the first field has been selected, or if non-delimited lines
     must be suppressed and the first field has *not* been selected.
     That is because a non-delimited line has exactly one field.  */
  buffer_first_field = (suppress_non_delimited ^ !print_kth (1));

  while (true)
    {
      if (field_idx == 1 && buffer_first_field)
        {
          ssize_t len;
          size_t n_bytes;

          len = getndelim2 (&field_1_buffer, &field_1_bufsize, 0,
                            GETNLINE_NO_LIMIT, delim, line_delim, stream);
          if (len < 0)
            {
              free (field_1_buffer);
              field_1_buffer = nullptr;
              if (ferror (stream) || feof (stream))
                break;
              xalloc_die ();
            }

          n_bytes = len;
          affirm (n_bytes != 0);

          c = 0;

          /* If the first field extends to the end of line (it is not
             delimited) and we are printing all non-delimited lines,
             print this one.  */
          if (to_uchar (field_1_buffer[n_bytes - 1]) != delim)
            {
              if (suppress_non_delimited)
                {
                  /* Empty.  */
                }
              else
                {
                  if (fwrite (field_1_buffer, sizeof (char), n_bytes, stdout)
                      != n_bytes)
                    write_error ();
                  /* Make sure the output line is newline terminated.  */
                  if (field_1_buffer[n_bytes - 1] != line_delim)
                    {
                      if (putchar (line_delim) < 0)
                        write_error ();
                    }
                  c = line_delim;
                }
              continue;
            }

          if (print_kth (1))
            {
              /* Print the field, but not the trailing delimiter.  */
              if (fwrite (field_1_buffer, sizeof (char), n_bytes - 1, stdout)
                  != n_bytes - 1)
                write_error ();

              /* With -d$'\n' don't treat the last '\n' as a delimiter.  */
              if (delim == line_delim)
                {
                  int last_c = getc (stream);
                  if (last_c != EOF)
                    {
                      ungetc (last_c, stream);
                      found_any_selected_field = true;
                    }
                }
              else
                {
                  found_any_selected_field = true;
                }
            }
          next_item (&field_idx);
        }

      int prev_c = c;

      if (print_kth (field_idx))
        {
          if (found_any_selected_field)
            {
              if (fwrite (output_delimiter_string, sizeof (char),
                          output_delimiter_length, stdout)
                  != output_delimiter_length)
                write_error ();
            }
          found_any_selected_field = true;

          while ((c = getc (stream)) != delim && c != line_delim && c != EOF)
            {
              if (putchar (c) < 0)
                write_error ();
              prev_c = c;
            }
        }
      else
        {
          while ((c = getc (stream)) != delim && c != line_delim && c != EOF)
            prev_c = c;
        }

      /* With -d$'\n' don't treat the last '\n' as a delimiter.  */
      if (delim == line_delim && c == delim)
        {
          int last_c = getc (stream);
          if (last_c != EOF)
            ungetc (last_c, stream);
          else
            c = last_c;
        }

      if (c == delim)
        next_item (&field_idx);
      else if (c == line_delim || c == EOF)
        {
          if (found_any_selected_field
              || !(suppress_non_delimited && field_idx == 1))
            {
              /* Make sure the output line is newline terminated.  */
              if (c == line_delim || prev_c != line_delim
                  || delim == line_delim)
                {
                  if (putchar (line_delim) < 0)
                    write_error ();
                }
            }
          if (c == EOF)
            break;

          /* Start processing the next input line.  */
          field_idx = 1;
          current_rp = frp;
          found_any_selected_field = false;
        }
    }
}

/* Process file FILE to standard output, using CUT_STREAM.
   Return true if successful.  */

static bool
cut_file (char const *file, void (*cut_stream) (FILE *))
{
  FILE *stream;

  if (STREQ (file, "-"))
    {
      have_read_stdin = true;
      stream = stdin;
      assume (stream);  /* Pacify GCC bug#109613.  */
    }
  else
    {
      stream = fopen (file, "r");
      if (stream == nullptr)
        {
          error (0, errno, "%s", quotef (file));
          return false;
        }
    }

  fadvise (stream, FADVISE_SEQUENTIAL);

  cut_stream (stream);

  int err = errno;
  if (!ferror (stream))
    err = 0;
  if (STREQ (file, "-"))
    clearerr (stream);		/* Also clear EOF.  */
  else if (fclose (stream) == EOF)
    err = errno;
  if (err)
    {
      error (0, err, "%s", quotef (file));
      return false;
    }
  return true;
}

int
main (int argc, char **argv)
{
  int optc;
  bool ok;
  bool delim_specified = false;
  bool byte_mode = false;
  char *spec_list_string = nullptr;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  /* By default, all non-delimited lines are printed.  */
  suppress_non_delimited = false;

  delim = '\0';
  have_read_stdin = false;

  while ((optc = getopt_long (argc, argv, "b:c:d:f:nsz", longopts, nullptr))
         != -1)
    {
      switch (optc)
        {
        case 'b':
        case 'c':
          /* Build the byte list.  */
          byte_mode = true;
          FALLTHROUGH;
        case 'f':
          /* Build the field list.  */
          if (spec_list_string)
            FATAL_ERROR (_("only one list may be specified"));
          spec_list_string = optarg;
          break;

        case 'd':
          /* New delimiter.  */
          /* Interpret -d '' to mean 'use the NUL byte as the delimiter.'  */
          if (optarg[0] != '\0' && optarg[1] != '\0')
            FATAL_ERROR (_("the delimiter must be a single character"));
          delim = optarg[0];
          delim_specified = true;
          break;

        case OUTPUT_DELIMITER_OPTION:
          /* Interpret --output-delimiter='' to mean
             'use the NUL byte as the delimiter.'  */
          output_delimiter_length = (optarg[0] == '\0'
                                     ? 1 : strlen (optarg));
          output_delimiter_string = optarg;
          break;

        case 'n':
          break;

        case 's':
          suppress_non_delimited = true;
          break;

        case 'z':
          line_delim = '\0';
          break;

        case COMPLEMENT_OPTION:
          complement = true;
          break;

        case_GETOPT_HELP_CHAR;
        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
        default:
          usage (EXIT_FAILURE);
        }
    }

  if (!spec_list_string)
    FATAL_ERROR (_("you must specify a list of bytes, characters, or fields"));

  if (byte_mode)
    {
      if (delim_specified)
        FATAL_ERROR (_("an input delimiter may be specified only\
 when operating on fields"));

      if (suppress_non_delimited)
        FATAL_ERROR (_("suppressing non-delimited lines makes sense\n\
\tonly when operating on fields"));
    }

  set_fields (spec_list_string,
              ((byte_mode ? SETFLD_ERRMSG_USE_POS : 0)
               | (complement ? SETFLD_COMPLEMENT : 0)));

  if (!delim_specified)
    delim = '\t';

  if (output_delimiter_string == nullptr)
    {
      output_delimiter_default[0] = delim;
      output_delimiter_string = output_delimiter_default;
      output_delimiter_length = 1;
    }

  void (*cut_stream) (FILE *) = byte_mode ? cut_bytes : cut_fields;
  if (optind == argc)
    ok = cut_file ("-", cut_stream);
  else
    for (ok = true; optind < argc; optind++)
      ok &= cut_file (argv[optind], cut_stream);


  if (have_read_stdin && fclose (stdin) == EOF)
    {
      error (0, errno, "-");
      ok = false;
    }

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
