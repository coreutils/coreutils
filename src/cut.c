/* cut - remove parts of lines of files
   Copyright (C) 1997-2026 Free Software Foundation, Inc.
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

#include "argmatch.h"
#include "assure.h"
#include "fadvise.h"
#include "getndelim2.h"
#include "ioblksize.h"
#include "mbbuf.h"

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


/* Pointer inside RP.  When checking if a -b,-c,-f is selected
   by a finite range, we check if it is between CURRENT_RP.LO
   and CURRENT_RP.HI.  If the index is greater than
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
static idx_t field_1_bufsize;

/* If true, do not output lines containing no delimiter characters.
   Otherwise, all such lines are printed.  This option is valid only
   with field mode.  */
static bool suppress_non_delimited;

/* If true, print all bytes, characters, or fields _except_
   those that were specified.  */
static bool complement;

/* The delimiter byte for single-byte field mode.  */
static unsigned char delim;

/* The delimiter character for multibyte field mode.  */
static mcel_t delim_mcel;

/* The delimiter bytes.  */
static char delim_bytes[MB_LEN_MAX];

/* The length of DELIM_BYTES.  */
static size_t delim_length = 1;

/* The delimiter for each line/record.  */
static unsigned char line_delim = '\n';

/* The length of output_delimiter_string.  */
static size_t output_delimiter_length;

/* The output field separator string.  Defaults to the 1-character
   string consisting of the input delimiter.  */
static char *output_delimiter_string;

/* The output delimiter string contents, if the default.  */
static char output_delimiter_default[MB_LEN_MAX];

/* True if we have ever read standard input.  */
static bool have_read_stdin;

/* If true, don't split multibyte characters in byte mode.  */
static bool no_split;

/* If true, interpret each run of whitespace as one field delimiter.  */
static bool whitespace_delimited;

/* If true, ignore leading and trailing whitespace in -w mode.  */
static bool trim_outer_whitespace;

/* If true, default the output delimiter to a single space.  */
static bool space_output_delimiter_default;

enum whitespace_option
{
  WHITESPACE_OPTION_TRIMMED
};

static char const *const whitespace_option_args[] =
{
  "trimmed", NULL
};

static enum whitespace_option const whitespace_option_types[] =
{
  WHITESPACE_OPTION_TRIMMED
};

ARGMATCH_VERIFY (whitespace_option_args, whitespace_option_types);

/* Whether to cut bytes, characters, or fields.  */
static enum
{
  CUT_MODE_NONE,
  CUT_MODE_BYTES,
  CUT_MODE_CHARACTERS,
  CUT_MODE_FIELDS
} cut_mode = CUT_MODE_NONE;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  COMPLEMENT_OPTION = CHAR_MAX + 1
};

static struct option const longopts[] =
{
  {"bytes", required_argument, NULL, 'b'},
  {"characters", required_argument, NULL, 'c'},
  {"fields", required_argument, NULL, 'f'},
  {"delimiter", required_argument, NULL, 'd'},
  {"no-partial", no_argument, NULL, 'n'},
  {"whitespace-delimited", optional_argument, NULL, 'w'},
  {"only-delimited", no_argument, NULL, 's'},
  {"output-delimiter", required_argument, NULL, 'O'},
  {"complement", no_argument, NULL, COMPLEMENT_OPTION},
  {"zero-terminated", no_argument, NULL, 'z'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
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

      oputs (_("\
  -b, --bytes=LIST\n\
         select only these byte positions\n\
"));
      oputs (_("\
  -c, --characters=LIST\n\
         select only these character positions\n\
"));
      oputs (_("\
      --complement\n\
         complement the set of selected bytes, characters or fields\n\
"));
      oputs (_("\
  -d, --delimiter=DELIM\n\
         use DELIM instead of TAB for field delimiter\n\
"));
      oputs (_("\
  -f, --fields=LIST\n\
         select only these fields;  also print any line that contains\n\
         no delimiter character, unless the -s option is specified\n\
"));
      oputs (_("\
  -F LIST\n\
         like -f, but also implies -w and -O ' '\n\
"));
      oputs (_("\
  -n, --no-partial\n\
         with -b, don't output partial multi-byte characters\n\
"));
      oputs (_("\
  -O, --output-delimiter=STRING\n\
         use STRING as the output delimiter;\n\
         the default is to use the input delimiter\n\
"));
      oputs (_("\
  -s, --only-delimited\n\
         do not print lines not containing delimiters\n\
"));
      oputs (_("\
  -w, --whitespace-delimited[=trimmed]\n\
         use runs of whitespace as the field delimiter;\n\
         with 'trimmed', ignore leading and trailing whitespace\n\
"));
      oputs (_("\
  -z, --zero-terminated\n\
         line delimiter is NUL, not newline\n\
"));
      oputs (HELP_OPTION_DESCRIPTION);
      oputs (VERSION_OPTION_DESCRIPTION);
      fputs (_("\
\n\
Use one, and only one of -b, -c, -f or -F.  Each LIST is made up of one\n\
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

static inline bool
single_byte_field_delim_ok (void)
{
  return delim_length == 1
         && (MB_CUR_MAX <= 1 || to_uchar (delim_bytes[0]) < 0x30);
}

/* Return true if the current charset is UTF-8.  */
static bool
is_utf8_charset (void)
{
  static int is_utf8 = -1;
  if (is_utf8 == -1)
    {
      char32_t w;
      mbstate_t mbs = {0};
      is_utf8 = mbrtoc32 (&w, "\xe2\x9f\xb8", 3, &mbs) == 3 && w == 0x27F8;
    }
  return is_utf8;
}

static inline bool
utf8_field_delim_ok (void)
{
  return ! delim_mcel.err && is_utf8_charset ();
}

static inline bool
field_delim_eq (mcel_t g)
{
  return delim_mcel.err ? g.err == delim_mcel.err : mcel_eq (g, delim_mcel);
}

enum field_terminator
{
  FIELD_DATA,
  FIELD_DELIMITER,
  FIELD_LINE_DELIMITER,
  FIELD_EOF
};

struct mbfield_parser
{
  bool whitespace_delimited;
  bool trim_outer_whitespace;
  bool at_line_start;
  bool have_saved;
  mcel_t saved_g;
};

static inline mcel_t
mbbuf_get_saved_char (mbbuf_t *mbbuf, bool *have_saved, mcel_t *saved_g)
{
  if (*have_saved)
    {
      *have_saved = false;
      return *saved_g;
    }
  return mbbuf_get_char (mbbuf);
}

static inline enum field_terminator
skip_whitespace_run (mbbuf_t *mbuf, struct mbfield_parser *parser,
                     bool *have_pending_line,
                     bool have_initial_whitespace)
{
  mcel_t g;

  do
    {
      g = mbbuf_get_char (mbuf);
      if (g.ch != MBBUF_EOF)
        *have_pending_line = true;
    }
  while (g.ch != MBBUF_EOF && g.ch != line_delim && c32issep (g.ch));

  bool trim_start = parser->trim_outer_whitespace && parser->at_line_start;

  if (parser->trim_outer_whitespace
      && (g.ch == MBBUF_EOF || g.ch == line_delim))
    {
      return g.ch == MBBUF_EOF ? FIELD_EOF : FIELD_LINE_DELIMITER;
    }

  parser->saved_g = g;
  parser->have_saved = true;
  return trim_start && !have_initial_whitespace ? FIELD_DATA : FIELD_DELIMITER;
}

static void
write_bytes (char const *buf, size_t n_bytes)
{
  if (fwrite (buf, sizeof (char), n_bytes, stdout) != n_bytes)
    write_error ();
}

static inline void
write_line_delim (void)
{
  if (putchar (line_delim) < 0)
    write_error ();
}

static inline void
reset_item_line (uintmax_t *item_idx, bool *print_delimiter)
{
  write_line_delim ();
  *item_idx = 0;
  *print_delimiter = false;
  current_rp = frp;
}

static inline void
write_pending_line_delim (uintmax_t item_idx)
{
  if (item_idx > 0)
    write_line_delim ();
}

static inline void
write_selected_item (bool *print_delimiter, bool range_start,
                     char const *buf, size_t n_bytes)
{
  if (output_delimiter_string != output_delimiter_default)
    {
      if (*print_delimiter && range_start)
        write_bytes (output_delimiter_string, output_delimiter_length);
      *print_delimiter = true;
    }

  write_bytes (buf, n_bytes);
}

static inline mcel_t
mbfield_get_char (mbbuf_t *mbbuf, struct mbfield_parser *parser)
{
  return (parser->whitespace_delimited
          ? mbbuf_get_saved_char (mbbuf, &parser->have_saved, &parser->saved_g)
          : mbbuf_get_char (mbbuf));
}

static inline enum field_terminator
mbfield_terminator (mbbuf_t *mbbuf, struct mbfield_parser *parser, mcel_t g,
                    bool *have_pending_line)
{
  if (g.ch == line_delim)
    return FIELD_LINE_DELIMITER;

  if (parser->whitespace_delimited)
    return (c32issep (g.ch)
            ? skip_whitespace_run (mbbuf, parser, have_pending_line, true)
            : FIELD_DATA);

  return field_delim_eq (g) ? FIELD_DELIMITER : FIELD_DATA;
}

static inline void
append_field_1_chunk (char const *buf, idx_t len, idx_t *n_bytes)
{
  if (field_1_bufsize - *n_bytes < len)
    {
      field_1_buffer = xpalloc (field_1_buffer, &field_1_bufsize,
                                len, -1, sizeof *field_1_buffer);
    }

  memcpy (field_1_buffer + *n_bytes, buf, len);
  *n_bytes += len;
}

static inline void
append_field_1_bytes (mbbuf_t *mbbuf, mcel_t g, idx_t *n_bytes)
{
  append_field_1_chunk (mbbuf_char_offset (mbbuf, g), g.len, n_bytes);
}

static enum field_terminator
scan_mb_field (mbbuf_t *mbbuf, struct mbfield_parser *parser,
               bool *have_pending_line, bool write_field, idx_t *n_bytes)
{
  if (parser->whitespace_delimited
      && parser->trim_outer_whitespace
      && parser->at_line_start)
    {
      enum field_terminator terminator
        = skip_whitespace_run (mbbuf, parser, have_pending_line, false);
      if (terminator != FIELD_DATA)
        return terminator;
    }

  parser->at_line_start = false;

  while (true)
    {
      mcel_t g = mbfield_get_char (mbbuf, parser);
      if (g.ch == MBBUF_EOF)
        return FIELD_EOF;

      *have_pending_line = true;

      enum field_terminator terminator
        = mbfield_terminator (mbbuf, parser, g, have_pending_line);
      if (terminator != FIELD_DATA)
        return terminator;

      if (n_bytes)
        append_field_1_bytes (mbbuf, g, n_bytes);
      else if (write_field)
        write_bytes (mbbuf_char_offset (mbbuf, g), g.len);
    }
}

/* Return a pointer to the next field delimiter in the UTF-8 record BUF,
   searching LEN bytes.  Return NULL if none is found.  DELIM_BYTES must
   represent a valid UTF-8 character.
   Like mbsmbchr() in numfmt but handles NUL bytes.  */
ATTRIBUTE_PURE
static char *
find_utf8_field_delim (char const *buf, size_t len)
{
#if 0
  unsigned char delim_0 = delim_bytes[0];
  if (delim_0 < 0x80)
    return memchr ((void *) buf, delim_0, len);
#endif
  return memmem (buf, len, delim_bytes, delim_length);
}

static inline char *
find_utf8_field_terminator (char const *buf, idx_t len, bool *is_delim)
{
  char *line_end = memchr ((void *) buf, line_delim, len);
  idx_t line_len = line_end ? line_end - buf : len;
  char *field_end = find_utf8_field_delim (buf, line_len);

  if (field_end)
    {
      *is_delim = true;
      return field_end;
    }

  *is_delim = false;
  return line_end;
}

static inline bool
begin_utf8_field (uintmax_t field_idx, bool buffer_first_field,
                  bool *found_any_selected_field)
{
  bool write_field = print_kth (field_idx);

  if (write_field && ! (field_idx == 1 && buffer_first_field))
    {
      if (*found_any_selected_field)
        write_bytes (output_delimiter_string, output_delimiter_length);
      *found_any_selected_field = true;
    }

  return write_field;
}

static inline void
reset_field_line (uintmax_t *field_idx, bool *found_any_selected_field,
                  bool *have_pending_line, struct mbfield_parser *parser)
{
  *field_idx = 1;
  current_rp = frp;
  *found_any_selected_field = false;
  *have_pending_line = false;
  parser->have_saved = false;
  parser->at_line_start = true;
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
        reset_item_line (&byte_idx, &print_delimiter);
      else if (c == EOF)
        {
          write_pending_line_delim (byte_idx);
          break;
        }
      else
        {
          next_item (&byte_idx);
          if (print_kth (byte_idx))
            {
              char ch = c;
              write_selected_item (&print_delimiter,
                                   is_range_start_index (byte_idx), &ch, 1);
            }
        }
    }
}

/* Read from STREAM, printing selected bytes without splitting
   multibyte characters.  */

static void
cut_bytes_no_split (FILE *stream)
{
  uintmax_t byte_idx = 0;
  bool print_delimiter = false;
  static char line_in[IO_BUFSIZE];
  mbbuf_t mbbuf;

  current_rp = frp;
  mbbuf_init (&mbbuf, line_in, sizeof line_in, stream);

  while (true)
    {
      mcel_t g = mbbuf_get_char (&mbbuf);

      if (g.ch == line_delim)
        reset_item_line (&byte_idx, &print_delimiter);
      else if (g.ch == MBBUF_EOF)
        {
          write_pending_line_delim (byte_idx);
          break;
        }
      else
        {
          bool first_selected_is_range_start = false;
          bool seen_selected = false;
          bool suffix_selected = true;

          for (idx_t i = 0; i < g.len; i++)
            {
              next_item (&byte_idx);
              if (print_kth (byte_idx))
                {
                  if (!seen_selected)
                    {
                      seen_selected = true;
                      first_selected_is_range_start
                        = is_range_start_index (byte_idx);
                    }
                }
              else if (seen_selected)
                suffix_selected = false;
            }

          if (seen_selected && suffix_selected)
            write_selected_item (&print_delimiter,first_selected_is_range_start,
                                 mbbuf_char_offset (&mbbuf, g), g.len);
        }
    }
}

/* Read from STREAM, printing to standard output any selected characters.  */

static void
cut_characters (FILE *stream)
{
  uintmax_t char_idx = 0;
  bool print_delimiter = false;
  static char line_in[IO_BUFSIZE];
  mbbuf_t mbbuf;

  current_rp = frp;
  mbbuf_init (&mbbuf, line_in, sizeof line_in, stream);

  while (true)
    {
      mcel_t g = mbbuf_get_char (&mbbuf);

      if (g.ch == line_delim)
        reset_item_line (&char_idx, &print_delimiter);
      else if (g.ch == MBBUF_EOF)
        {
          write_pending_line_delim (char_idx);
          break;
        }
      else
        {
          next_item (&char_idx);
          if (print_kth (char_idx))
            write_selected_item (&print_delimiter,
                                 is_range_start_index (char_idx),
                                 mbbuf_char_offset (&mbbuf, g), g.len);
        }
    }
}

/* Read from STREAM, printing to standard output any selected fields,
   using a multibyte-aware field delimiter parser.  */

static void
cut_fields_mb_any (FILE *stream, bool whitespace_mode)
{
  static char line_in[IO_BUFSIZE];
  mbbuf_t mbbuf;
  struct mbfield_parser parser =
    {
      .whitespace_delimited = whitespace_mode,
      .trim_outer_whitespace = trim_outer_whitespace,
      .at_line_start = true,
      .saved_g = { .ch = MBBUF_EOF }
    };
  uintmax_t field_idx = 1;
  bool found_any_selected_field = false;
  bool buffer_first_field;
  bool have_pending_line = false;

  current_rp = frp;
  mbbuf_init (&mbbuf, line_in, sizeof line_in, stream);

  buffer_first_field = (suppress_non_delimited ^ !print_kth (1));

  while (true)
    {
      if (field_idx == 1 && buffer_first_field)
        {
          idx_t n_bytes = 0;
          enum field_terminator terminator
            = scan_mb_field (&mbbuf, &parser, &have_pending_line, false,
                             &n_bytes);
          if (terminator == FIELD_EOF && n_bytes == 0)
            return;

          if (terminator != FIELD_DELIMITER)
            {
              if (!suppress_non_delimited)
                {
                  write_bytes (field_1_buffer, n_bytes);
                  write_line_delim ();
                }

              if (terminator == FIELD_EOF)
                break;

              reset_field_line (&field_idx, &found_any_selected_field,
                                &have_pending_line, &parser);
              continue;
            }

          if (print_kth (1))
            {
              write_bytes (field_1_buffer, n_bytes);
              found_any_selected_field = true;
            }
          next_item (&field_idx);
        }

      enum field_terminator terminator;
      bool write_field = print_kth (field_idx);

      if (write_field)
        {
          if (found_any_selected_field)
            write_bytes (output_delimiter_string, output_delimiter_length);
          found_any_selected_field = true;
        }

      terminator = scan_mb_field (&mbbuf, &parser, &have_pending_line,
                                  write_field, NULL);

      if (terminator == FIELD_DELIMITER)
        next_item (&field_idx);
      else
        {
          if (terminator == FIELD_EOF && !have_pending_line)
            break;
          if (found_any_selected_field
              || !(suppress_non_delimited && field_idx == 1))
            write_line_delim ();
          if (terminator == FIELD_EOF)
            break;

          reset_field_line (&field_idx, &found_any_selected_field,
                            &have_pending_line, &parser);
        }
    }
}

/* Read from STREAM, printing to standard output any selected fields,
   using a multibyte field delimiter.  */

static void
cut_fields_mb (FILE *stream)
{
  cut_fields_mb_any (stream, false);
}

/* Read from STREAM, printing to standard output any selected fields,
   using UTF-8-aware byte searches for the field delimiter.  */

static void
cut_fields_mb_utf8 (FILE *stream)
{
  static char line_in[IO_BUFSIZE];
  mbbuf_t mbbuf;
  bool buffer_first_field;
  uintmax_t field_idx = 1;
  bool found_any_selected_field = false;
  bool have_pending_line = false;
  bool write_field;
  idx_t field_1_n_bytes = 0;
  idx_t overlap = delim_length - 1;

  current_rp = frp;
  buffer_first_field = suppress_non_delimited ^ !print_kth (1);
  mbbuf_init (&mbbuf, line_in, sizeof line_in, stream);
  write_field = begin_utf8_field (field_idx, buffer_first_field,
                                  &found_any_selected_field);

  while (true)
    {
      idx_t safe = mbbuf_utf8_safe_prefix (&mbbuf, overlap);
      idx_t processed = 0;

      if (safe == 0)
        {
          if (mbbuf_avail (&mbbuf) == 0)
            break;
          continue;
        }

      char *chunk = mbbuf.buffer + mbbuf.offset;

      while (processed < safe)
        {
          bool is_delim = false;
          char *terminator
            = find_utf8_field_terminator (chunk + processed, safe - processed,
                                          &is_delim);
          idx_t field_len = terminator ? terminator - (chunk + processed)
                                       : safe - processed;

          if (field_len != 0 || terminator)
            have_pending_line = true;

          if (field_idx == 1 && buffer_first_field)
            append_field_1_chunk (chunk + processed, field_len,
                                  &field_1_n_bytes);
          else if (write_field)
            write_bytes (chunk + processed, field_len);
          processed += field_len;

          if (!terminator)
            break;

          if (is_delim)
            {
              if (field_idx == 1 && buffer_first_field)
                {
                  if (print_kth (1))
                    {
                      write_bytes (field_1_buffer, field_1_n_bytes);
                      found_any_selected_field = true;
                    }
                  field_1_n_bytes = 0;
                }

              processed += delim_length;
              next_item (&field_idx);
              write_field = begin_utf8_field (field_idx, buffer_first_field,
                                              &found_any_selected_field);
            }
          else
            {
              processed++;

              if (field_idx == 1 && buffer_first_field)
                {
                  if (!suppress_non_delimited)
                    {
                      write_bytes (field_1_buffer, field_1_n_bytes);
                      write_line_delim ();
                    }
                  field_1_n_bytes = 0;
                }
              else if (found_any_selected_field
                       || !(suppress_non_delimited && field_idx == 1))
                write_line_delim ();

              field_idx = 1;
              current_rp = frp;
              found_any_selected_field = false;
              have_pending_line = false;
              write_field = begin_utf8_field (field_idx, buffer_first_field,
                                              &found_any_selected_field);
            }
        }

      mbbuf_advance (&mbbuf, processed);
    }

  if (!have_pending_line)
    return;

  if (field_idx == 1 && buffer_first_field)
    {
      if (field_1_n_bytes != 0 && !suppress_non_delimited)
        {
          write_bytes (field_1_buffer, field_1_n_bytes);
          write_line_delim ();
        }
    }
  else if (field_idx != 1 || found_any_selected_field)
    {
      if (found_any_selected_field
          || !(suppress_non_delimited && field_idx == 1))
        write_line_delim ();
    }
}

/* Read from STREAM, printing to standard output any selected fields,
   using runs of whitespace as the field delimiter.  */

static void
cut_fields_ws (FILE *stream)
{
  cut_fields_mb_any (stream, true);
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
          size_t field_1_bufsize_s = field_1_bufsize;

          len = getndelim2 (&field_1_buffer, &field_1_bufsize_s, 0,
                            GETNLINE_NO_LIMIT, delim, line_delim, stream);
          field_1_bufsize = field_1_bufsize_s;
          if (len < 0)
            {
              free (field_1_buffer);
              field_1_buffer = NULL;
              field_1_bufsize = 0;
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
                  write_bytes (field_1_buffer, n_bytes);
                  /* Make sure the output line is newline terminated. */
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
              write_bytes (field_1_buffer, n_bytes - 1);

              /* With -d$'\n' don't treat the last '\n' as a delim.  */
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
      bool write_field = print_kth (field_idx);

      if (write_field)
        {
          if (found_any_selected_field)
            write_bytes (output_delimiter_string, output_delimiter_length);
          found_any_selected_field = true;
        }

      while ((c = getc (stream)) != delim && c != line_delim && c != EOF)
        {
          if (write_field && putchar (c) < 0)
            write_error ();
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

  if (streq (file, "-"))
    {
      have_read_stdin = true;
      stream = stdin;
      assume (stream);  /* Pacify GCC bug#109613.  */
    }
  else
    {
      stream = fopen (file, "r");
      if (stream == NULL)
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
  if (streq (file, "-"))
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
  bool whitespace_delimited_specified = false;
  char *spec_list_string = NULL;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, "b:c:d:f:F:nO:szw", longopts, NULL))
         != -1)
    {
      switch (optc)
        {
        case 'b':
          cut_mode = CUT_MODE_BYTES;
          FALLTHROUGH;
        case 'c':
          if (optc == 'c')
            cut_mode = CUT_MODE_CHARACTERS;
          FALLTHROUGH;
        case 'f':
        case 'F':
          if (optc == 'f' || optc == 'F')
            cut_mode = CUT_MODE_FIELDS;
          if (optc == 'F')
            {
              whitespace_delimited = true;
              space_output_delimiter_default = true;
            }
          if (spec_list_string)
            FATAL_ERROR (_("only one list may be specified"));
          spec_list_string = optarg;
          break;

        case 'd':
          /* New delimiter.  */
          /* Interpret -d '' to mean 'use the NUL byte as the delimiter.'  */
          if (optarg[0] == '\0')
            {
              delim = '\0';
              delim_bytes[0] = '\0';
              delim_length = 1;
            }
          else if (MB_CUR_MAX <= 1)
            {
              if (optarg[1] != '\0')
                FATAL_ERROR (_("the delimiter must be a single character"));
              delim = optarg[0];
              delim_bytes[0] = optarg[0];
              delim_length = 1;
            }
          else
            {
              mcel_t g = mcel_scanz (optarg);
              if (optarg[g.len] != '\0')
                FATAL_ERROR (_("the delimiter must be a single character"));
              memcpy (delim_bytes, optarg, g.len);
              delim_length = g.len;
              delim_mcel = g;
              if (g.len == 1)
                delim = optarg[0];
            }
          delim_specified = true;
          break;

        case 'w':
          whitespace_delimited = true;
          whitespace_delimited_specified = true;
          trim_outer_whitespace
            = (optarg
               && XARGMATCH ("--whitespace-delimited", optarg,
                             whitespace_option_args,
                             whitespace_option_types)
                  == WHITESPACE_OPTION_TRIMMED);
          break;

        case 'O':
          /* Interpret --output-delimiter='' to mean
             'use the NUL byte as the delimiter.'  */
          output_delimiter_length = (optarg[0] == '\0'
                                     ? 1 : strlen (optarg));
          output_delimiter_string = optarg;
          break;

        case 'n':
          no_split = true;
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

  if (cut_mode == CUT_MODE_BYTES || cut_mode == CUT_MODE_CHARACTERS)
    {
      if (delim_specified || whitespace_delimited)
        FATAL_ERROR (_("an input delimiter makes sense\n\
\tonly when operating on fields"));

      if (suppress_non_delimited)
        FATAL_ERROR (_("suppressing non-delimited lines makes sense\n\
\tonly when operating on fields"));
    }

  if (delim_specified && whitespace_delimited_specified)
    FATAL_ERROR (_("-d and -w are mutually exclusive"));

  if (delim_specified && !whitespace_delimited_specified)
    whitespace_delimited = false;

  set_fields (spec_list_string,
              (((cut_mode == CUT_MODE_BYTES
                 || cut_mode == CUT_MODE_CHARACTERS)
                ? SETFLD_ERRMSG_USE_POS : 0)
               | (complement ? SETFLD_COMPLEMENT : 0)));

  if (!delim_specified)
    {
      delim = '\t';
      delim_bytes[0] = '\t';
      delim_length = 1;
      delim_mcel = mcel_ch ('\t', 1);
    }

  if (output_delimiter_string == NULL)
    {
      output_delimiter_string = output_delimiter_default;
      if (space_output_delimiter_default)
        {
          output_delimiter_default[0] = ' ';
          output_delimiter_length = 1;
        }
      else
        {
          memcpy (output_delimiter_default, delim_bytes, delim_length);
          output_delimiter_length = delim_length;
        }
    }

  void (*cut_stream) (FILE *) = NULL;
  switch (cut_mode)
    {
    case CUT_MODE_NONE:
      unreachable ();

    case CUT_MODE_BYTES:
      cut_stream = MB_CUR_MAX <= 1 || !no_split
                   ? cut_bytes : cut_bytes_no_split;
      break;

    case CUT_MODE_CHARACTERS:
      cut_stream = MB_CUR_MAX <= 1 ? cut_bytes : cut_characters;
      break;

    case CUT_MODE_FIELDS:
      cut_stream = whitespace_delimited ? cut_fields_ws
                   : single_byte_field_delim_ok () ? cut_fields
                   : utf8_field_delim_ok () ? cut_fields_mb_utf8
                   : cut_fields_mb;
      break;
    }
  affirm (cut_stream);
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
