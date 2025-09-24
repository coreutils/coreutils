/* wc - print the number of lines, words, and bytes in files
   Copyright (C) 1985-2025 Free Software Foundation, Inc.

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

/* Written by Paul Rubin, phr@ocf.berkeley.edu
   and David MacKenzie, djm@gnu.ai.mit.edu. */

#include <config.h>

#include <ctype.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include <argmatch.h>
#include <argv-iter.h>
#include <fadvise.h>
#include <physmem.h>
#include <readtokens0.h>
#include <stat-size.h>
#include <xbinary-io.h>

#include "system.h"
#include "cpu-supports.h"
#include "ioblksize.h"
#include "wc.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "wc"

#define AUTHORS \
  proper_name ("Paul Rubin"), \
  proper_name ("David MacKenzie")

static bool wc_isprint[UCHAR_MAX + 1];
static bool wc_isspace[UCHAR_MAX + 1];

static bool debug;

/* Cumulative number of lines, words, chars and bytes in all files so far.
   max_line_length is the maximum over all files processed so far.  */
static uintmax_t total_lines;
static uintmax_t total_words;
static uintmax_t total_chars;
static uintmax_t total_bytes;
static bool total_lines_overflow;
static bool total_words_overflow;
static bool total_chars_overflow;
static bool total_bytes_overflow;
static intmax_t max_line_length;

/* Which counts to print. */
static bool print_lines, print_words, print_chars, print_bytes;
static bool print_linelength;

/* The print width of each count.  */
static int number_width;

/* True if we have ever read the standard input. */
static bool have_read_stdin;

/* Used to determine if file size can be determined without reading.  */
static idx_t page_size;

/* Enable to _not_ treat non breaking space as a word separator.  */
static bool posixly_correct;

/* The result of calling fstat or stat on a file descriptor or file.  */
struct fstatus
{
  /* If positive, fstat or stat has not been called yet.  Otherwise,
     this is the value returned from fstat or stat.  */
  int failed;

  /* If FAILED is zero, this is the file's status.  */
  struct stat st;
};

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  DEBUG_PROGRAM_OPTION = CHAR_MAX + 1,
  FILES0_FROM_OPTION,
  TOTAL_OPTION,
};

static struct option const longopts[] =
{
  {"bytes", no_argument, nullptr, 'c'},
  {"chars", no_argument, nullptr, 'm'},
  {"lines", no_argument, nullptr, 'l'},
  {"words", no_argument, nullptr, 'w'},
  {"debug", no_argument, nullptr, DEBUG_PROGRAM_OPTION},
  {"files0-from", required_argument, nullptr, FILES0_FROM_OPTION},
  {"max-line-length", no_argument, nullptr, 'L'},
  {"total", required_argument, nullptr, TOTAL_OPTION},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
};

enum total_type
  {
    total_auto,         /* 0: default or --total=auto */
    total_always,       /* 1: --total=always */
    total_only,         /* 2: --total=only */
    total_never         /* 3: --total=never */
  };
static char const *const total_args[] =
{
  "auto", "always", "only", "never", nullptr
};
static enum total_type const total_types[] =
{
  total_auto, total_always, total_only, total_never
};
ARGMATCH_VERIFY (total_args, total_types);
static enum total_type total_mode = total_auto;

#ifdef USE_AVX2_WC_LINECOUNT
static bool
avx2_supported (void)
{
  bool avx2_enabled = cpu_supports ("avx2");
  if (debug)
    error (0, 0, (avx2_enabled
                  ? _("using avx2 hardware support")
                  : _("avx2 support not detected")));

  return avx2_enabled;
}
#endif

#ifdef USE_AVX512_WC_LINECOUNT
static bool
avx512_supported (void)
{
  bool avx512_enabled = (cpu_supports ("avx512f")
                         && cpu_supports ("avx512bw"));

  if (debug)
    error (0, 0, (avx512_enabled
                  ? _("using avx512 hardware support")
                  : _("avx512 support not detected")));

  return avx512_enabled;
}
#endif

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE]...\n\
  or:  %s [OPTION]... --files0-from=F\n\
"),
              program_name, program_name);
      fputs (_("\
Print newline, word, and byte counts for each FILE, and a total line if\n\
more than one FILE is specified.  A word is a nonempty sequence of non white\n\
space delimited by white space characters or by start or end of input.\n\
"), stdout);

      emit_stdin_note ();

      fputs (_("\
\n\
The options below may be used to select which counts are printed, always in\n\
the following order: newline, word, character, byte, maximum line length.\n\
  -c, --bytes            print the byte counts\n\
  -m, --chars            print the character counts\n\
  -l, --lines            print the newline counts\n\
"), stdout);
      fputs (_("\
      --files0-from=F    read input from the files specified by\n\
                           NUL-terminated names in file F;\n\
                           If F is - then read names from standard input\n\
  -L, --max-line-length  print the maximum display width\n\
  -w, --words            print the word counts\n\
"), stdout);
      fputs (_("\
      --total=WHEN       when to print a line with total counts;\n\
                           WHEN can be: auto, always, only, never\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* Return non zero if POSIXLY_CORRECT is not set and WC is a non breaking
   space.  */
ATTRIBUTE_PURE
static int
maybe_c32isnbspace (char32_t wc)
{
  return ! posixly_correct && c32isnbspace (wc);
}

/* FILE is the name of the file (or null for standard input)
   associated with the specified counters.  */
static void
write_counts (uintmax_t lines,
              uintmax_t words,
              uintmax_t chars,
              uintmax_t bytes,
              intmax_t linelength,
              char const *file)
{
  static char const format_sp_int[] = " %*s";
  char const *format_int = format_sp_int + 1;
  char buf[MAX (INT_BUFSIZE_BOUND (intmax_t),
                INT_BUFSIZE_BOUND (uintmax_t))];

  if (print_lines)
    {
      printf (format_int, number_width, umaxtostr (lines, buf));
      format_int = format_sp_int;
    }
  if (print_words)
    {
      printf (format_int, number_width, umaxtostr (words, buf));
      format_int = format_sp_int;
    }
  if (print_chars)
    {
      printf (format_int, number_width, umaxtostr (chars, buf));
      format_int = format_sp_int;
    }
  if (print_bytes)
    {
      printf (format_int, number_width, umaxtostr (bytes, buf));
      format_int = format_sp_int;
    }
  if (print_linelength)
    printf (format_int, number_width, imaxtostr (linelength, buf));
  if (file)
    printf (" %s", strchr (file, '\n') ? quotef (file) : file);
  putchar ('\n');
}

/* Read FD and return a summary.  */
static struct wc_lines
wc_lines (int fd)
{
#ifdef USE_AVX512_WC_LINECOUNT
  static signed char use_avx512;
  if (!use_avx512)
    use_avx512 = avx512_supported () ? 1 : -1;
  if (0 < use_avx512)
    return wc_lines_avx512 (fd);
#endif
#ifdef USE_AVX2_WC_LINECOUNT
  static signed char use_avx2;
  if (!use_avx2)
    use_avx2 = avx2_supported () ? 1 : -1;
  if (0 < use_avx2)
    return wc_lines_avx2 (fd);
#endif

  intmax_t lines = 0, bytes = 0;
  bool long_lines = false;

  while (true)
    {
      char buf[IO_BUFSIZE + 1];
      ssize_t bytes_read = read (fd, buf, IO_BUFSIZE);
      if (bytes_read <= 0)
        return (struct wc_lines) { bytes_read == 0 ? 0 : errno, lines, bytes };

      bytes += bytes_read;
      char *end = buf + bytes_read;
      idx_t buflines = 0;

      if (! long_lines)
        {
          /* Avoid function call overhead for shorter lines.  */
          for (char *p = buf; p < end; p++)
            buflines += *p == '\n';
        }
      else
        {
          /* rawmemchr is more efficient with longer lines.  */
          *end = '\n';
          for (char *p = buf; (p = rawmemchr (p, '\n')) < end; p++)
            buflines++;
        }

      /* If the average line length in the block is >= 15, then use
          memchr for the next block, where system specific optimizations
          may outweigh function call overhead.
          FIXME: This line length was determined in 2015, on both
          x86_64 and ppc64, but it's worth re-evaluating in future with
          newer compilers, CPUs, or memchr() implementations etc.  */
      long_lines = 15 * buflines <= bytes_read;
      lines += buflines;
    }
}

/* Count words.  FILE_X is the name of the file (or null for standard
   input) that is open on descriptor FD.  *FSTATUS is its status.
   Return true if successful.  */
static bool
wc (int fd, char const *file_x, struct fstatus *fstatus)
{
  int err = 0;
  char buf[IO_BUFSIZE + 1];
  intmax_t lines, words, chars, bytes, linelength;
  bool count_bytes, count_chars, count_complicated;
  char const *file = file_x ? file_x : _("standard input");

  lines = words = chars = bytes = linelength = 0;

  /* If in the current locale, chars are equivalent to bytes, we prefer
     counting bytes, because that's easier.  */
  if (MB_CUR_MAX > 1)
    {
      count_bytes = print_bytes;
      count_chars = print_chars;
    }
  else
    {
      count_bytes = print_bytes || print_chars;
      count_chars = false;
    }
  count_complicated = print_words || print_linelength;

  /* Advise the kernel of our access pattern only if we will read().  */
  if (!count_bytes || count_chars || print_lines || count_complicated)
    fdadvise (fd, 0, 0, FADVISE_SEQUENTIAL);

  /* When counting only bytes, save some line- and word-counting
     overhead.  If FD is a 'regular' Unix file, using lseek is enough
     to get its 'size' in bytes.  Otherwise, read blocks of IO_BUFSIZE
     bytes at a time until EOF.  Note that the 'size' (number of bytes)
     that wc reports is smaller than stats.st_size when the file is not
     positioned at its beginning.  That's why the lseek calls below are
     necessary.  For example the command
     '(dd ibs=99k skip=1 count=0; ./wc -c) < /etc/group'
     should make wc report '0' bytes.  */

  if (count_bytes && !count_chars && !print_lines && !count_complicated)
    {
      bool skip_read = false;

      if (0 < fstatus->failed)
        fstatus->failed = fstat (fd, &fstatus->st);

      /* For sized files, seek to one st_blksize before EOF rather than to EOF.
         This works better for files in proc-like file systems where
         the size is only approximate.  */
      if (! fstatus->failed && usable_st_size (&fstatus->st)
          && 0 <= fstatus->st.st_size)
        {
          off_t end_pos = fstatus->st.st_size;
          off_t current_pos = lseek (fd, 0, SEEK_CUR);
          if (current_pos < 0)
            ;
          else if (end_pos % page_size)
            {
              /* We only need special handling of /proc and /sys files etc.
                 when they're a multiple of PAGE_SIZE.  In the common case
                 for files with st_size not a multiple of PAGE_SIZE,
                 it's more efficient and accurate to use st_size.

                 Be careful here.  The current position may actually be
                 beyond the end of the file.  As in the example above.  */

              bytes = end_pos < current_pos ? 0 : end_pos - current_pos;
              if (bytes && 0 <= lseek (fd, bytes, SEEK_CUR))
                skip_read = true;
              else
                bytes = 0;
            }
          else
            {
              off_t hi_pos = (end_pos
                              - end_pos % (STP_BLKSIZE (&fstatus->st) + 1));
              if (0 <= current_pos && current_pos < hi_pos
                  && 0 <= lseek (fd, hi_pos, SEEK_CUR))
                bytes = hi_pos - current_pos;
            }
        }

      if (! skip_read)
        {
          fdadvise (fd, 0, 0, FADVISE_SEQUENTIAL);
          for (ssize_t bytes_read;
               (bytes_read = read (fd, buf, IO_BUFSIZE));
               bytes += bytes_read)
            if (bytes_read < 0)
              {
                err = errno;
                break;
              }
        }
    }
  else if (!count_chars && !count_complicated)
    {
      /* Use a separate loop when counting only lines or lines and bytes --
         but not chars or words.  */
      struct wc_lines w = wc_lines (fd);
      err = w.err;
      lines = w.lines;
      bytes = w.bytes;
    }
  else if (MB_CUR_MAX > 1)
    {
      bool in_word = false;
      intmax_t linepos = 0;
      mbstate_t state; mbszero (&state);
      bool in_shift = false;
      idx_t prev = 0; /* Number of bytes carried over from previous round.  */

      for (ssize_t bytes_read;
           ((bytes_read = read (fd, buf + prev, IO_BUFSIZE - prev))
            || prev);
           )
        {
          if (bytes_read < 0)
            {
              err = errno;
              break;
            }

          bytes += bytes_read;
          char const *p = buf;
          char const *plim = p + prev + bytes_read;
          do
            {
              char32_t wide_char;
              idx_t charbytes;
              bool single_byte;

              if (!in_shift && 0 <= *p && *p < 0x80)
                {
                  /* Handle most ASCII characters quickly, without calling
                     mbrtoc32.  */
                  charbytes = 1;
                  wide_char = *p;
                  single_byte = true;
                }
              else
                {
                  idx_t scanbytes = plim - (p + prev);
                  size_t n = mbrtoc32 (&wide_char, p + prev, scanbytes, &state);
                  prev = 0;

                  if (scanbytes < n)
                    {
                      if (n == (size_t) -2 && plim - p < IO_BUFSIZE
                          && bytes_read)
                        {
                          /* An incomplete character that is not ridiculously
                             long and there may be more input.  Move the bytes
                             to buffer start and prepare to read more data.  */
                          prev = plim - p;
                          memmove (buf, p, prev);
                          in_shift = true;
                          break;
                        }

                      /* Remember that we read a byte, but don't complain
                         about the error.  Because of the decoding error,
                         this is a considered to be byte but not a
                         character (that is, chars is not incremented).  */
                      p++;
                      mbszero (&state);
                      in_shift = false;

                      /* Treat encoding errors as non white space.
                         POSIX says a word is "a non-zero-length string of
                         characters delimited by white space".  This is
                         wrong in some sense, as the string can be delimited
                         by start or end of input, and it is unclear what it
                         means when the input contains encoding errors.
                         Since encoding errors are not white space,
                         treat them that way here.  */
                      words += !in_word;
                      in_word = true;
                      continue;
                    }

                  charbytes = n + !n;
                  single_byte = charbytes == !in_shift;
                  in_shift = !mbsinit (&state);
                }

              switch (wide_char)
                {
                case '\n':
                  lines++;
                  FALLTHROUGH;
                case '\r':
                case '\f':
                  if (linepos > linelength)
                    linelength = linepos;
                  linepos = 0;
                  in_word = false;
                  break;

                case '\t':
                  linepos += 8 - (linepos % 8);
                  in_word = false;
                  break;

                case ' ':
                  linepos++;
                  FALLTHROUGH;
                case '\v':
                  in_word = false;
                  break;

                default:;
                  bool in_word2;
                  if (single_byte)
                    {
                      linepos += wc_isprint[wide_char];
                      in_word2 = !wc_isspace[wide_char];
                    }
                  else
                    {
                      /* c32width can be expensive on macOS for example,
                         so avoid if not needed.  */
                      if (print_linelength)
                        {
                          int width = c32width (wide_char);
                          if (width > 0)
                            linepos += width;
                        }
                      in_word2 = (! c32isspace (wide_char)
                                  && ! maybe_c32isnbspace (wide_char));
                    }

                  /* Count words by counting word starts, i.e., each
                     white space character (or the start of input)
                     followed by non white space.  */
                  words += !in_word & in_word2;
                  in_word = in_word2;
                  break;
                }

              p += charbytes;
              chars++;
            }
          while (p < plim);
        }
      if (linepos > linelength)
        linelength = linepos;
    }
  else
    {
      bool in_word = false;
      intmax_t linepos = 0;

      for (ssize_t bytes_read; (bytes_read = read (fd, buf, IO_BUFSIZE)); )
        {
          if (bytes_read < 0)
            {
              err = errno;
              break;
            }

          bytes += bytes_read;
          char const *p = buf;
          do
            {
              unsigned char c = *p++;
              switch (c)
                {
                case '\n':
                  lines++;
                  FALLTHROUGH;
                case '\r':
                case '\f':
                  if (linepos > linelength)
                    linelength = linepos;
                  linepos = 0;
                  in_word = false;
                  break;

                case '\t':
                  linepos += 8 - (linepos % 8);
                  in_word = false;
                  break;

                case ' ':
                  linepos++;
                  FALLTHROUGH;
                case '\v':
                  in_word = false;
                  break;

                default:
                  linepos += wc_isprint[c];
                  bool in_word2 = !wc_isspace[c];
                  words += !in_word & in_word2;
                  in_word = in_word2;
                  break;
                }
            }
          while (--bytes_read);
        }
      if (linepos > linelength)
        linelength = linepos;
    }

  if (count_chars < print_chars)
    chars = bytes;

  if (total_mode != total_only)
    write_counts (lines, words, chars, bytes, linelength, file_x);

  total_lines_overflow |= ckd_add (&total_lines, total_lines, lines);
  total_words_overflow |= ckd_add (&total_words, total_words, words);
  total_chars_overflow |= ckd_add (&total_chars, total_chars, chars);
  total_bytes_overflow |= ckd_add (&total_bytes, total_bytes, bytes);

  if (linelength > max_line_length)
    max_line_length = linelength;

  if (err)
    error (0, err, "%s", quotef (file));
  return !err;
}

static bool
wc_file (char const *file, struct fstatus *fstatus)
{
  if (! file || streq (file, "-"))
    {
      have_read_stdin = true;
      xset_binary_mode (STDIN_FILENO, O_BINARY);
      return wc (STDIN_FILENO, file, fstatus);
    }
  else
    {
      int fd = open (file, O_RDONLY | O_BINARY);
      if (fd == -1)
        {
          error (0, errno, "%s", quotef (file));
          return false;
        }
      else
        {
          bool ok = wc (fd, file, fstatus);
          if (close (fd) != 0)
            {
              error (0, errno, "%s", quotef (file));
              return false;
            }
          return ok;
        }
    }
}

/* Return the file status for the NFILES files addressed by FILE.
   Optimize the case where only one number is printed, for just one
   file; in that case we can use a print width of 1, so we don't need
   to stat the file.  Handle the case of (nfiles == 0) in the same way;
   that happens when we don't know how long the list of file names will be.  */

static struct fstatus *
get_input_fstatus (idx_t nfiles, char *const *file)
{
  struct fstatus *fstatus = xnmalloc (nfiles ? nfiles : 1, sizeof *fstatus);

  if (nfiles == 0
      || (nfiles == 1
          && ((print_lines + print_words + print_chars
               + print_bytes + print_linelength)
              == 1)))
    fstatus[0].failed = 1;
  else
    {
      for (idx_t i = 0; i < nfiles; i++)
        fstatus[i].failed = (! file[i] || streq (file[i], "-")
                             ? fstat (STDIN_FILENO, &fstatus[i].st)
                             : stat (file[i], &fstatus[i].st));
    }

  return fstatus;
}

/* Return a print width suitable for the NFILES files whose status is
   recorded in FSTATUS.  Optimize the same special case that
   get_input_fstatus optimizes.  */

ATTRIBUTE_PURE
static int
compute_number_width (idx_t nfiles, struct fstatus const *fstatus)
{
  int width = 1;

  if (0 < nfiles && fstatus[0].failed <= 0)
    {
      int minimum_width = 1;
      uintmax_t regular_total = 0;

      for (idx_t i = 0; i < nfiles; i++)
        if (! fstatus[i].failed)
          {
            if (!S_ISREG (fstatus[i].st.st_mode))
              minimum_width = 7;
            else if (ckd_add (&regular_total, regular_total,
                              fstatus[i].st.st_size))
              {
                regular_total = UINTMAX_MAX;
                break;
              }
          }

      for (; 10 <= regular_total; regular_total /= 10)
        width++;
      if (width < minimum_width)
        width = minimum_width;
    }

  return width;
}


int
main (int argc, char **argv)
{
  int optc;
  idx_t nfiles;
  char **files;
  char *files_from = nullptr;
  struct fstatus *fstatus;
  struct Tokens tok;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  page_size = getpagesize ();
  /* Line buffer stdout to ensure lines are written atomically and immediately
     so that processes running in parallel do not intersperse their output.  */
  setvbuf (stdout, nullptr, _IOLBF, 0);

  posixly_correct = (getenv ("POSIXLY_CORRECT") != nullptr);

  print_lines = print_words = print_chars = print_bytes = false;
  print_linelength = false;
  total_lines = total_words = total_chars = total_bytes = max_line_length = 0;

  while ((optc = getopt_long (argc, argv, "clLmw", longopts, nullptr)) != -1)
    switch (optc)
      {
      case 'c':
        print_bytes = true;
        break;

      case 'm':
        print_chars = true;
        break;

      case 'l':
        print_lines = true;
        break;

      case 'w':
        print_words = true;
        break;

      case 'L':
        print_linelength = true;
        break;

      case DEBUG_PROGRAM_OPTION:
        debug = true;
        break;

      case FILES0_FROM_OPTION:
        files_from = optarg;
        break;

      case TOTAL_OPTION:
        total_mode = XARGMATCH ("--total", optarg, total_args, total_types);
        break;

      case_GETOPT_HELP_CHAR;

      case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

      default:
        usage (EXIT_FAILURE);
      }

  if (! (print_lines || print_words || print_chars || print_bytes
         || print_linelength))
    print_lines = print_words = print_bytes = true;

  if (print_linelength)
    for (int i = 0; i <= UCHAR_MAX; i++)
      wc_isprint[i] = !!isprint (i);
  if (print_words)
    for (int i = 0; i <= UCHAR_MAX; i++)
      wc_isspace[i] = isspace (i) || maybe_c32isnbspace (btoc32 (i));

  bool read_tokens = false;
  struct argv_iterator *ai;
  if (files_from)
    {
      FILE *stream;

      /* When using --files0-from=F, you may not specify any files
         on the command-line.  */
      if (optind < argc)
        {
          error (0, 0, _("extra operand %s"), quoteaf (argv[optind]));
          fprintf (stderr, "%s\n",
                   _("file operands cannot be combined with --files0-from"));
          usage (EXIT_FAILURE);
        }

      if (streq (files_from, "-"))
        stream = stdin;
      else
        {
          stream = fopen (files_from, "r");
          if (stream == nullptr)
            error (EXIT_FAILURE, errno, _("cannot open %s for reading"),
                   quoteaf (files_from));
        }

      /* Read the file list into RAM if we can detect its size and that
         size is reasonable.  Otherwise, we'll read a name at a time.  */
      struct stat st;
      if (fstat (fileno (stream), &st) == 0
          && S_ISREG (st.st_mode)
          && st.st_size <= MIN (10 * 1024 * 1024, physmem_available () / 2))
        {
          read_tokens = true;
          readtokens0_init (&tok);
          if (! readtokens0 (stream, &tok) || fclose (stream) != 0)
            error (EXIT_FAILURE, 0, _("cannot read file names from %s"),
                   quoteaf (files_from));
          files = tok.tok;
          nfiles = tok.n_tok;
          ai = argv_iter_init_argv (files);
        }
      else
        {
          files = nullptr;
          nfiles = 0;
          ai = argv_iter_init_stream (stream);
        }
    }
  else
    {
      static char *stdin_only[] = { nullptr };
      files = (optind < argc ? argv + optind : stdin_only);
      nfiles = (optind < argc ? argc - optind : 1);
      ai = argv_iter_init_argv (files);
    }

  if (!ai)
    xalloc_die ();

  fstatus = get_input_fstatus (nfiles, files);
  if (total_mode == total_only)
    number_width = 1;  /* No extra padding, since no alignment requirement.  */
  else
    number_width = compute_number_width (nfiles, fstatus);

  bool ok = true;
  enum argv_iter_err ai_err;
  char *file_name;
  for (int i = 0; (file_name = argv_iter (ai, &ai_err)); i++)
    {
      bool skip_file = false;
      if (files_from && streq (files_from, "-") && streq (file_name, "-"))
        {
          /* Give a better diagnostic in an unusual case:
             printf - | wc --files0-from=- */
          error (0, 0, _("when reading file names from standard input, "
                         "no file name of %s allowed"),
                 quoteaf (file_name));
          skip_file = true;
        }

      if (!file_name[0])
        {
          /* Diagnose a zero-length file name.  When it's one
             among many, knowing the record number may help.
             FIXME: currently print the record number only with
             --files0-from=FILE.  Maybe do it for argv, too?  */
          if (files_from == nullptr)
            error (0, 0, "%s", _("invalid zero-length file name"));
          else
            {
              /* Using the standard 'filename:line-number:' prefix here is
                 not totally appropriate, since NUL is the separator, not NL,
                 but it might be better than nothing.  */
              error (0, 0, "%s:%zu: %s", quotef (files_from),
                     argv_iter_n_args (ai), _("invalid zero-length file name"));
            }
          skip_file = true;
        }

      if (skip_file)
        ok = false;
      else
        ok &= wc_file (file_name, &fstatus[nfiles ? i : 0]);

      if (! nfiles)
        fstatus[0].failed = 1;
    }
  switch (ai_err)
    {
    case AI_ERR_EOF:
      break;

    case AI_ERR_READ:
      error (0, errno, _("%s: read error"), quotef (files_from));
      ok = false;
      break;

    case AI_ERR_MEM:
      xalloc_die ();

    case AI_ERR_OK: default:
      unreachable ();
    }

  /* No arguments on the command line is fine.  That means read from stdin.
     However, no arguments on the --files0-from input stream is an error
     means don't read anything.  */
  if (ok && !files_from && argv_iter_n_args (ai) == 0)
    ok &= wc_file (nullptr, &fstatus[0]);

  if (read_tokens)
    readtokens0_free (&tok);

  if (total_mode != total_never
      && (total_mode != total_auto || 1 < argv_iter_n_args (ai)))
    {
      if (total_lines_overflow)
        {
          total_lines = UINTMAX_MAX;
          error (0, EOVERFLOW, _("total lines"));
          ok = false;
        }
      if (total_words_overflow)
        {
          total_words = UINTMAX_MAX;
          error (0, EOVERFLOW, _("total words"));
          ok = false;
        }
      if (total_chars_overflow)
        {
          total_chars = UINTMAX_MAX;
          error (0, EOVERFLOW, _("total characters"));
          ok = false;
        }
      if (total_bytes_overflow)
        {
          total_bytes = UINTMAX_MAX;
          error (0, EOVERFLOW, _("total bytes"));
          ok = false;
        }

      write_counts (total_lines, total_words, total_chars, total_bytes,
                    max_line_length,
                    total_mode != total_only ? _("total") : nullptr);
    }

  argv_iter_free (ai);

  free (fstatus);

  if (have_read_stdin && close (STDIN_FILENO) != 0)
    error (EXIT_FAILURE, errno, "-");

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
