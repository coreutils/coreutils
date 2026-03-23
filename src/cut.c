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
#include "c-ctype.h"
#include "fadvise.h"
#include "getndelim2.h"
#include "ioblksize.h"
#include "mbbuf.h"
#include "memchr2.h"

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
         use a run of blank characters as the field delimiter;\n\
         with 'trimmed', ignore leading and trailing blanks\n\
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
field_delim_is_line_delim (void)
{
  return delim_length == 1 && delim_bytes[0] == line_delim;
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

static bool
mcel_isblank (mcel_t g)
{
  /* This is faster than calling c32issep directly.
     Assume all unibyte locales match c_isblank.  */
  return (g.len == 1 && c_isblank (g.ch)) || (g.len > 1 && c32issep (g.ch));
}

static inline bool
bytesearch_field_delim_ok (void)
{
  unsigned char delim_0 = delim_bytes[0];

  return (delim_length == 1
          ? (MB_CUR_MAX <= 1
             || (is_utf8_charset () ? delim_0 < 0x80 : delim_0 < 0x30))
          : utf8_field_delim_ok ());
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
  FIELD_FINAL_DELIMITER,
  FIELD_EOF
};

enum bytesearch_mode
{
  BYTESEARCH_FIELDS,
  BYTESEARCH_LINE_ONLY
};

struct bytesearch_context
{
  enum bytesearch_mode mode;
  bool blank_delimited;
  char *line_end;
  bool line_end_known;
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
  while (g.ch != MBBUF_EOF && g.ch != line_delim && mcel_isblank (g));

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
  /* Avoid a function call for smaller amounts,
     using instead the macro to directly interact with the stdio buffer.  */
  if (n_bytes <= 4)
    {
      for (size_t i = 0; i < n_bytes; i++)
        if (putchar (buf[i]) < 0)
          write_error ();
      return;
    }

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
    return (mcel_isblank(g)
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

/* Return a pointer to the next field delimiter in BUF, searching LEN bytes.
   Return NULL if none is found.  DELIM_BYTES must be a single byte or
   represent a valid UTF-8 character.  BUF can contain invalid/NUL bytes,
   and must have room for a trailing NUL byte at BUF[LEN].  */
ATTRIBUTE_PURE
static char *
find_bytesearch_field_delim (char *buf, size_t len)
{
#if ! __GLIBC__  /* Only S390 has optimized memmem on glibc-2.42  */
  return memmem (buf, len, delim_bytes, delim_length);
#else
  unsigned char delim_0 = delim_bytes[0];
  if (delim_length == 1)
    return memchr ((void *) buf, delim_0, len);

  char const *p = buf;
  char const *end = buf + len;

  char saved = buf[len];
  buf[len] = '\0'; /* for strstr.  */

  while (p < end)
    {
      char const *nul = memchr (p, '\0', end - p);
      if (!nul)
        {
          char *match = strstr (p, delim_bytes);
          buf[len] = saved;
          return match;
        }

      if (p < nul)
        {
          char *match = strstr (p, delim_bytes);
          if (match)
            {
              buf[len] = saved;
              return match;
            }
        }

      p = nul + 1;
    }

  buf[len] = saved;
  return NULL;
#endif
}

static inline enum field_terminator
find_bytesearch_field_terminator (char *buf, idx_t len, bool at_eof,
                                  struct bytesearch_context *ctx,
                                  char **terminator)
{
  if (ctx->mode == BYTESEARCH_LINE_ONLY)
    {
      if (!ctx->line_end_known)
        {
          ctx->line_end = memchr ((void *) buf, line_delim, len);
          ctx->line_end_known = true;
        }

      *terminator = ctx->line_end;
      return *terminator ? FIELD_LINE_DELIMITER : FIELD_DATA;
    }

  if (field_delim_is_line_delim ())
    {
      *terminator = memchr ((void *) buf, line_delim, len);
      if (!*terminator)
        return FIELD_DATA;

      return (at_eof && *terminator + 1 == buf + len
              ? FIELD_FINAL_DELIMITER
              : FIELD_DELIMITER);
    }

  if (!ctx->line_end_known)
    {
      ctx->line_end = memchr ((void *) buf, line_delim, len);
      ctx->line_end_known = true;
    }

  idx_t field_len = ctx->line_end ? ctx->line_end - buf : len;

  char *field_end = (ctx->blank_delimited
                     ? memchr2 (buf, ' ', '\t', field_len)
                     : find_bytesearch_field_delim (buf, field_len));

  if (field_end)
    {
      *terminator = field_end;
      return FIELD_DELIMITER;
    }

  *terminator = ctx->line_end;
  return ctx->line_end ? FIELD_LINE_DELIMITER : FIELD_DATA;
}

static inline bool
begin_field_output (uintmax_t field_idx, bool buffer_first_field,
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

static inline idx_t
bytesearch_safe_prefix (mbbuf_t *mbbuf, idx_t overlap)
{
  idx_t available = mbbuf_fill (mbbuf, overlap + 1);
  if (available == 0)
    return 0;

  if (feof (mbbuf->fp))
    return available;

  return overlap < available ? available - overlap : 0;
}

static inline bool
field_selection_exhausted (uintmax_t field_idx)
{
  return !print_kth (field_idx) && current_rp->lo == UINTMAX_MAX;
}

static inline void
sync_byte_selection (uintmax_t byte_idx)
{
  while (current_rp->hi <= byte_idx)
    current_rp++;
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
  uintmax_t byte_idx = 0;
  bool print_delimiter = false;
  static char line_in[IO_BUFSIZE];

  current_rp = frp;

  while (true)
    {
      idx_t available = fread (line_in, sizeof *line_in, sizeof line_in,
                               stream);
      if (available == 0)
        {
          write_pending_line_delim (byte_idx);
          break;
        }

      idx_t processed = 0;

      while (processed < available)
        {
          char *line = line_in + processed;
          char *line_end = memchr ((void *) line, line_delim,
                                   available - processed);
          char *end = line + (line_end ? line_end - line : available - processed);
          char *p = line;

          while (p < end)
            {
              sync_byte_selection (byte_idx);

              if (byte_idx + 1 < current_rp->lo)
                {
                  idx_t skip = MIN (end - p, current_rp->lo - (byte_idx + 1));
                  p += skip;
                  byte_idx += skip;
                }
              else
                {
                  idx_t n = MIN (end - p, current_rp->hi - byte_idx);
                  write_selected_item (&print_delimiter,
                                       is_range_start_index (byte_idx + 1),
                                       p, n);
                  p += n;
                  byte_idx += n;
                }
            }

          processed += end - line;
          if (line_end)
            {
              processed++;
              reset_item_line (&byte_idx, &print_delimiter);
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
      bool write_field = begin_field_output (field_idx, buffer_first_field,
                                             &found_any_selected_field);

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
   using byte searches for a single-byte or valid UTF-8 field delimiter.  */

static void
cut_fields_bytesearch (FILE *stream)
{
  /* Leave 1 byte unused so space to NUL terminate (for strstr).  */
  static char line_in[IO_BUFSIZE+1];
  mbbuf_t mbbuf;
  bool buffer_first_field;
  uintmax_t field_idx = 1;
  bool found_any_selected_field = false;
  bool have_pending_line = false;
  bool skip_line_remainder = false;
  bool skip_blank_run = false;
  bool write_field;
  idx_t field_1_n_bytes = 0;
  idx_t overlap = whitespace_delimited ? 0 : delim_length - 1;

  current_rp = frp;
  buffer_first_field = suppress_non_delimited ^ !print_kth (1);
  mbbuf_init (&mbbuf, line_in, sizeof line_in - 1, stream);
  write_field = begin_field_output (field_idx, buffer_first_field,
                                    &found_any_selected_field);

  while (true)
    {
      idx_t safe = bytesearch_safe_prefix (&mbbuf, overlap);
      idx_t processed = 0;

      if (safe == 0)
        {
          if (mbbuf_avail (&mbbuf) == 0)
            break;
          continue;
        }

      char *chunk = mbbuf.buffer + mbbuf.offset;
      struct bytesearch_context search =
        {
          .mode = BYTESEARCH_FIELDS,
          .blank_delimited = whitespace_delimited
        };

      /* Shortcut the case were there is no delimiter in input,
         as directly outputting without parsing is 20x faster.  */
      if (field_idx == 1
          && !suppress_non_delimited && !whitespace_delimited
          && !field_delim_is_line_delim ()
          && !have_pending_line
          && field_1_n_bytes == 0
          && !skip_line_remainder
          && !find_bytesearch_field_delim (chunk, safe))
        {
          char *last_line_delim = feof (mbbuf.fp) ? chunk + safe - 1
                                  : memrchr ((void *) chunk, line_delim, safe);
          if (last_line_delim)
            {
              idx_t n = last_line_delim - chunk + 1;
              write_bytes (chunk, n);
              if (feof (mbbuf.fp) && chunk[n - 1] != line_delim)
                write_line_delim ();
              mbbuf_advance (&mbbuf, n);
              if (feof (mbbuf.fp))
                return;
              continue;
            }
        }

      while (processed < safe)
        {
          char *terminator = NULL;

          if (skip_blank_run)
            {
              while (processed < safe && c_isblank (chunk[processed]))
                processed++;

              if (processed == safe)
                break;

              skip_blank_run = false;
            }

          if (skip_line_remainder)
            {
              search.mode = BYTESEARCH_LINE_ONLY;
              enum field_terminator terminator_kind
                = find_bytesearch_field_terminator (chunk + processed,
                                                    safe - processed,
                                                    feof (mbbuf.fp), &search,
                                                    &terminator);
              if (terminator_kind == FIELD_LINE_DELIMITER)
                {
                  processed = terminator - chunk + 1;
                  if (found_any_selected_field
                      || !(suppress_non_delimited && field_idx == 1))
                    write_line_delim ();
                  field_idx = 1;
                  current_rp = frp;
                  found_any_selected_field = false;
                  have_pending_line = false;
                  skip_line_remainder = false;
                  search.line_end = NULL;
                  search.line_end_known = false;
                  search.mode = BYTESEARCH_FIELDS;
                  write_field = begin_field_output (field_idx,
                                                    buffer_first_field,
                                                    &found_any_selected_field);
                  continue;
                }

              processed = safe;
              if (terminator_kind == FIELD_DATA && feof (mbbuf.fp))
                {
                  if (found_any_selected_field
                      || !(suppress_non_delimited && field_idx == 1))
                    write_line_delim ();
                  mbbuf_advance (&mbbuf, processed);
                  return;
                }
              break;
            }

          enum field_terminator terminator_kind;
          search.mode = BYTESEARCH_FIELDS;
          terminator_kind
            = find_bytesearch_field_terminator (chunk + processed,
                                                safe - processed,
                                                feof (mbbuf.fp), &search,
                                                &terminator);
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

          if (terminator_kind == FIELD_DATA)
            break;

          if (terminator_kind == FIELD_DELIMITER)
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

              processed += whitespace_delimited ? 1 : delim_length;
              next_item (&field_idx);
              write_field = begin_field_output (field_idx, buffer_first_field,
                                                &found_any_selected_field);
              if (field_selection_exhausted (field_idx))
                {
                  if (field_delim_is_line_delim ())
                    {
                      if (found_any_selected_field
                          || !(suppress_non_delimited && field_idx == 1))
                        write_line_delim ();
                      mbbuf_advance (&mbbuf, processed);
                      return;
                    }

                  skip_line_remainder = true;
                }
              else if (whitespace_delimited)
                skip_blank_run = true;
            }
          else if (terminator_kind == FIELD_LINE_DELIMITER)
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
              search.line_end = NULL;
              search.line_end_known = false;
              write_field = begin_field_output (field_idx, buffer_first_field,
                                                &found_any_selected_field);
            }
          else
            {
              affirm (terminator_kind == FIELD_FINAL_DELIMITER);
              processed++;

              if (field_idx == 1 && buffer_first_field)
                {
                  if (print_kth (1))
                    write_bytes (field_1_buffer, field_1_n_bytes);
                  field_1_n_bytes = 0;
                  next_item (&field_idx);
                }

              if (found_any_selected_field
                  || !(suppress_non_delimited && field_idx == 1))
                write_line_delim ();

              mbbuf_advance (&mbbuf, processed);
              return;
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
  if (MB_CUR_MAX <= 1 && !trim_outer_whitespace)
    cut_fields_bytesearch (stream);
  else
    cut_fields_mb_any (stream, true);
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
                   : bytesearch_field_delim_ok () ? cut_fields_bytesearch
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
