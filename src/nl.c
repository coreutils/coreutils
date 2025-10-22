/* nl -- number lines of files
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

/* Written by Scott Bartram (nancy!scott@uunet.uu.net)
   Revised by David MacKenzie (djm@gnu.ai.mit.edu) */

#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"

#include <regex.h>

#include "fadvise.h"
#include "linebuffer.h"
#include "quote.h"
#include "xdectoint.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "nl"

#define AUTHORS \
  proper_name ("Scott Bartram"), \
  proper_name ("David MacKenzie")

/* Line-number formats.  They are given an int width, an intmax_t
   value, and a string separator.  */

/* Right justified, no leading zeroes.  */
static char const FORMAT_RIGHT_NOLZ[] = "%*jd%s";

/* Right justified, leading zeroes.  */
static char const FORMAT_RIGHT_LZ[] = "%0*jd%s";

/* Left justified, no leading zeroes.  */
static char const FORMAT_LEFT[] = "%-*jd%s";

/* Default section delimiter characters.  */
static char DEFAULT_SECTION_DELIMITERS[] = "\\:";

/* Types of input lines: either one of the section delimiters,
   or text to output. */
enum section
{
  Header, Body, Footer, Text
};

/* Format of body lines (-b).  */
static char const *body_type = "t";

/* Format of header lines (-h).  */
static char const *header_type = "n";

/* Format of footer lines (-f).  */
static char const *footer_type = "n";

/* Format currently being used (body, header, or footer).  */
static char const *current_type;

/* Regex for body lines to number (-bp).  */
static struct re_pattern_buffer body_regex;

/* Regex for header lines to number (-hp).  */
static struct re_pattern_buffer header_regex;

/* Regex for footer lines to number (-fp).  */
static struct re_pattern_buffer footer_regex;

/* Fastmaps for the above.  */
static char body_fastmap[UCHAR_MAX + 1];
static char header_fastmap[UCHAR_MAX + 1];
static char footer_fastmap[UCHAR_MAX + 1];

/* Pointer to current regex, if any.  */
static struct re_pattern_buffer *current_regex = nullptr;

/* Separator string to print after line number (-s).  */
static char const *separator_str = "\t";

/* Input section delimiter string (-d).  */
static char *section_del = DEFAULT_SECTION_DELIMITERS;

/* Header delimiter string.  */
static char *header_del = nullptr;

/* Header section delimiter length.  */
static size_t header_del_len;

/* Body delimiter string.  */
static char *body_del = nullptr;

/* Body section delimiter length.  */
static size_t body_del_len;

/* Footer delimiter string.  */
static char *footer_del = nullptr;

/* Footer section delimiter length.  */
static size_t footer_del_len;

/* Input buffer.  */
static struct linebuffer line_buf;

/* printf format string for unnumbered lines.  */
static char *print_no_line_fmt = nullptr;

/* Starting line number on each page (-v).  */
static intmax_t starting_line_number = 1;

/* Line number increment (-i).  */
static intmax_t page_incr = 1;

/* If true, reset line number at start of each page (-p).  */
static bool reset_numbers = true;

/* Number of blank lines to consider to be one line for numbering (-l).  */
static intmax_t blank_join = 1;

/* Width of line numbers (-w).  */
static int lineno_width = 6;

/* Line number format (-n).  */
static char const *lineno_format = FORMAT_RIGHT_NOLZ;

/* Current print line number.  */
static intmax_t line_no;

/* Whether the current line number has incremented past limits.  */
static bool line_no_overflow;

/* True if we have ever read standard input.  */
static bool have_read_stdin;

static struct option const longopts[] =
{
  {"header-numbering", required_argument, nullptr, 'h'},
  {"body-numbering", required_argument, nullptr, 'b'},
  {"footer-numbering", required_argument, nullptr, 'f'},
  {"starting-line-number", required_argument, nullptr, 'v'},
  {"line-increment", required_argument, nullptr, 'i'},
  {"no-renumber", no_argument, nullptr, 'p'},
  {"join-blank-lines", required_argument, nullptr, 'l'},
  {"number-separator", required_argument, nullptr, 's'},
  {"number-width", required_argument, nullptr, 'w'},
  {"number-format", required_argument, nullptr, 'n'},
  {"section-delimiter", required_argument, nullptr, 'd'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
};

/* Print a usage message and quit. */

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
Write each FILE to standard output, with line numbers added.\n\
"), stdout);

      emit_stdin_note ();
      emit_mandatory_arg_note ();

      fputs (_("\
  -b, --body-numbering=STYLE      use STYLE for numbering body lines\n\
  -d, --section-delimiter=CC      use CC for logical page delimiters\n\
  -f, --footer-numbering=STYLE    use STYLE for numbering footer lines\n\
"), stdout);
      fputs (_("\
  -h, --header-numbering=STYLE    use STYLE for numbering header lines\n\
  -i, --line-increment=NUMBER     line number increment at each line\n\
  -l, --join-blank-lines=NUMBER   group of NUMBER empty lines counted as one\n\
  -n, --number-format=FORMAT      insert line numbers according to FORMAT\n\
  -p, --no-renumber               do not reset line numbers for each section\n\
  -s, --number-separator=STRING   add STRING after (possible) line number\n\
"), stdout);
      fputs (_("\
  -v, --starting-line-number=NUMBER  first line number for each section\n\
  -w, --number-width=NUMBER       use NUMBER columns for line numbers\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
Default options are: -bt -d'\\:' -fn -hn -i1 -l1 -n'rn' -s<TAB> -v1 -w6\n\
\n\
CC are two delimiter characters used to construct logical page delimiters;\n\
a missing second character implies ':'.  As a GNU extension one can specify\n\
more than two characters, and also specifying the empty string (-d '')\n\
disables section matching.\n\
"), stdout);
      fputs (_("\
\n\
STYLE is one of:\n\
\n\
  a      number all lines\n\
  t      number only nonempty lines\n\
  n      number no lines\n\
  pBRE   number only lines that contain a match for the basic regular\n\
         expression, BRE\n\
"), stdout);
      fputs (_("\
\n\
FORMAT is one of:\n\
\n\
  ln     left justified, no leading zeros\n\
  rn     right justified, no leading zeros\n\
  rz     right justified, leading zeros\n\
\n\
"), stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* Set the command line flag TYPEP and possibly the regex pointer REGEXP,
   according to 'optarg'.  */

static bool
build_type_arg (char const **typep,
                struct re_pattern_buffer *regexp, char *fastmap)
{
  char const *errmsg;
  bool rval = true;

  switch (*optarg)
    {
    case 'a':
    case 't':
    case 'n':
      *typep = optarg;
      break;
    case 'p':
      *typep = optarg++;
      regexp->buffer = nullptr;
      regexp->allocated = 0;
      regexp->fastmap = fastmap;
      regexp->translate = nullptr;
      re_syntax_options =
        RE_SYNTAX_POSIX_BASIC & ~RE_CONTEXT_INVALID_DUP & ~RE_NO_EMPTY_RANGES;
      errmsg = re_compile_pattern (optarg, strlen (optarg), regexp);
      if (errmsg)
        error (EXIT_FAILURE, 0, "%s", (errmsg));
      break;
    default:
      rval = false;
      break;
    }
  return rval;
}

/* Print the line number and separator; increment the line number. */

static void
print_lineno (void)
{
  if (line_no_overflow)
    error (EXIT_FAILURE, 0, _("line number overflow"));

  printf (lineno_format, lineno_width, line_no, separator_str);

  if (ckd_add (&line_no, line_no, page_incr))
    line_no_overflow = true;
}

static void
reset_lineno (void)
{
  if (reset_numbers)
    {
      line_no = starting_line_number;
      line_no_overflow = false;
    }
}

/* Switch to a header section. */

static void
proc_header (void)
{
  current_type = header_type;
  current_regex = &header_regex;
  reset_lineno ();
  putchar ('\n');
}

/* Switch to a body section. */

static void
proc_body (void)
{
  current_type = body_type;
  current_regex = &body_regex;
  reset_lineno ();
  putchar ('\n');
}

/* Switch to a footer section. */

static void
proc_footer (void)
{
  current_type = footer_type;
  current_regex = &footer_regex;
  reset_lineno ();
  putchar ('\n');
}

/* Process a regular text line in 'line_buf'. */

static void
proc_text (void)
{
  static intmax_t blank_lines = 0;	/* Consecutive blank lines so far. */

  switch (*current_type)
    {
    case 'a':
      if (blank_join > 1)
        {
          if (1 < line_buf.length || ++blank_lines == blank_join)
            {
              print_lineno ();
              blank_lines = 0;
            }
          else
            fputs (print_no_line_fmt, stdout);
        }
      else
        print_lineno ();
      break;
    case 't':
      if (1 < line_buf.length)
        print_lineno ();
      else
        fputs (print_no_line_fmt, stdout);
      break;
    case 'n':
      fputs (print_no_line_fmt, stdout);
      break;
    case 'p':
      switch (re_search (current_regex, line_buf.buffer, line_buf.length - 1,
                         0, line_buf.length - 1, nullptr))
        {
        case -2:
          error (EXIT_FAILURE, errno, _("error in regular expression search"));

        case -1:
          fputs (print_no_line_fmt, stdout);
          break;

        default:
          print_lineno ();
          break;
        }
    }
  fwrite (line_buf.buffer, sizeof (char), line_buf.length, stdout);
}

/* Return the type of line in 'line_buf'. */

static enum section
check_section (void)
{
  size_t len = line_buf.length - 1;

  if (len < 2 || footer_del_len < 2
      || !memeq (line_buf.buffer, section_del, 2))
    return Text;
  if (len == header_del_len
      && memeq (line_buf.buffer, header_del, header_del_len))
    return Header;
  if (len == body_del_len
      && memeq (line_buf.buffer, body_del, body_del_len))
    return Body;
  if (len == footer_del_len
      && memeq (line_buf.buffer, footer_del, footer_del_len))
    return Footer;
  return Text;
}

/* Read and process the file pointed to by FP. */

static void
process_file (FILE *fp)
{
  while (readlinebuffer (&line_buf, fp))
    {
      switch (check_section ())
        {
        case Header:
          proc_header ();
          break;
        case Body:
          proc_body ();
          break;
        case Footer:
          proc_footer ();
          break;
        case Text:
          proc_text ();
          break;
        }

      if (ferror (stdout))
        write_error ();
    }
}

/* Process file FILE to standard output.
   Return true if successful.  */

static bool
nl_file (char const *file)
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
      if (stream == nullptr)
        {
          error (0, errno, "%s", quotef (file));
          return false;
        }
    }

  fadvise (stream, FADVISE_SEQUENTIAL);

  process_file (stream);

  int err = errno;
  if (!ferror (stream))
    err = 0;
  if (streq (file, "-"))
    clearerr (stream);		/* Also clear EOF. */
  else if (fclose (stream) != 0 && !err)
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
  int c;
  size_t len;
  bool ok = true;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  have_read_stdin = false;

  while ((c = getopt_long (argc, argv, "h:b:f:v:i:pl:s:w:n:d:", longopts,
                           nullptr))
         != -1)
    {
      switch (c)
        {
        case 'h':
          if (! build_type_arg (&header_type, &header_regex, header_fastmap))
            {
              error (0, 0, _("invalid header numbering style: %s"),
                     quote (optarg));
              ok = false;
            }
          break;
        case 'b':
          if (! build_type_arg (&body_type, &body_regex, body_fastmap))
            {
              error (0, 0, _("invalid body numbering style: %s"),
                     quote (optarg));
              ok = false;
            }
          break;
        case 'f':
          if (! build_type_arg (&footer_type, &footer_regex, footer_fastmap))
            {
              error (0, 0, _("invalid footer numbering style: %s"),
                     quote (optarg));
              ok = false;
            }
          break;
        case 'v':
          starting_line_number = xdectoimax (optarg, INTMAX_MIN, INTMAX_MAX, "",
                                             _("invalid starting line number"),
                                             0);
          break;
        case 'i':
          page_incr = xdectoimax (optarg, INTMAX_MIN, INTMAX_MAX, "",
                                  _("invalid line number increment"), 0);
          break;
        case 'p':
          reset_numbers = false;
          break;
        case 'l':
          blank_join = xnumtoimax (optarg, 10, 1, INTMAX_MAX, "",
                                   _("invalid line number of blank lines"),
                                   0, XTOINT_MIN_RANGE | XTOINT_MAX_QUIET);
          break;
        case 's':
          separator_str = optarg;
          break;
        case 'w':
          lineno_width = xnumtoimax (optarg, 10, 1, INT_MAX, "",
                                     _("invalid line number field width"),
                                     0, XTOINT_MIN_RANGE);
          break;
        case 'n':
          if (streq (optarg, "ln"))
            lineno_format = FORMAT_LEFT;
          else if (streq (optarg, "rn"))
            lineno_format = FORMAT_RIGHT_NOLZ;
          else if (streq (optarg, "rz"))
            lineno_format = FORMAT_RIGHT_LZ;
          else
            {
              error (0, 0, _("invalid line numbering format: %s"),
                     quote (optarg));
              ok = false;
            }
          break;
        case 'd':
          len = strlen (optarg);
          if (len == 1 || len == 2)  /* POSIX.  */
            {
              char *p = section_del;
              while (*optarg)
                *p++ = *optarg++;
            }
          else
            section_del = optarg;  /* GNU extension.  */
          break;
        case_GETOPT_HELP_CHAR;
        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
        default:
          ok = false;
          break;
        }
    }

  if (!ok)
    usage (EXIT_FAILURE);

  /* Initialize the section delimiters.  */
  len = strlen (section_del);

  header_del_len = len * 3;
  header_del = xmalloc (header_del_len + 1);
  stpcpy (stpcpy (stpcpy (header_del, section_del), section_del), section_del);

  body_del_len = len * 2;
  body_del = header_del + len;

  footer_del_len = len;
  footer_del = body_del + len;

  /* Initialize the input buffer.  */
  initbuffer (&line_buf);

  /* Initialize the printf format for unnumbered lines. */
  len = strlen (separator_str);
  print_no_line_fmt = xmalloc (lineno_width + len + 1);
  memset (print_no_line_fmt, ' ', lineno_width + len);
  print_no_line_fmt[lineno_width + len] = '\0';

  line_no = starting_line_number;
  current_type = body_type;
  current_regex = &body_regex;

  /* Main processing. */

  if (optind == argc)
    ok = nl_file ("-");
  else
    for (; optind < argc; optind++)
      ok &= nl_file (argv[optind]);

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, "-");

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
