/* wc - print the number of bytes, words, and lines in files
   Copyright (C) 85, 91, 1995-2002 Free Software Foundation, Inc.

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

/* Written by Paul Rubin, phr@ocf.berkeley.edu
   and David MacKenzie, djm@gnu.ai.mit.edu. */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

/* Get mbstate_t, mbrtowc(), wcwidth().  */
#if HAVE_WCHAR_H
# include <wchar.h>
#endif

/* Get iswprint(), iswspace().  */
#if HAVE_WCTYPE_H
# include <wctype.h>
#endif
#if !defined iswprint && !HAVE_ISWPRINT
# define iswprint(wc) 1
#endif
#if !defined iswspace && !HAVE_ISWSPACE
# define iswspace(wc) \
    ((wc) == (unsigned char) (wc) && ISSPACE ((unsigned char) (wc)))
#endif

/* Include this after wctype.h so that we `#undef' ISPRINT
   (from Solaris's euc.h, from widec.h, from wctype.h) before
   redefining and using it. */
#include "system.h"

#include "closeout.h"
#include "error.h"
#include "inttostr.h"
#include "safe-read.h"

/* Some systems, like BeOS, have multibyte encodings but lack mbstate_t.  */
#if HAVE_MBRTOWC && defined mbstate_t
# define mbrtowc(pwc, s, n, ps) (mbrtowc) (pwc, s, n, 0)
#endif

#ifndef HAVE_DECL_WCWIDTH
"this configure-time declaration test was not run"
#endif
#if !HAVE_DECL_WCWIDTH
extern int wcwidth ();
#endif

/* If wcwidth() doesn't exist, assume all printable characters have
   width 1.  */
#if !defined wcwidth && !HAVE_WCWIDTH
# define wcwidth(wc) ((wc) == 0 ? 0 : iswprint (wc) ? 1 : -1)
#endif

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "wc"

#define AUTHORS N_ ("Paul Rubin and David MacKenzie")

/* Size of atomic reads. */
#define BUFFER_SIZE (16 * 1024)

/* The name this program was run with. */
char *program_name;

/* Cumulative number of lines, words, chars and bytes in all files so far.
   max_line_length is the maximum over all files processed so far.  */
static uintmax_t total_lines;
static uintmax_t total_words;
static uintmax_t total_chars;
static uintmax_t total_bytes;
static uintmax_t max_line_length;

/* Which counts to print. */
static int print_lines, print_words, print_chars, print_bytes;
static int print_linelength;

/* Nonzero if we have ever read the standard input. */
static int have_read_stdin;

/* The error code to return to the system. */
static int exit_status;

/* If nonzero, do not line up columns but instead separate numbers by
   a single space as specified in Single Unix Specification and POSIX. */
static int posixly_correct;

static struct option const longopts[] =
{
  {"bytes", no_argument, NULL, 'c'},
  {"chars", no_argument, NULL, 'm'},
  {"lines", no_argument, NULL, 'l'},
  {"words", no_argument, NULL, 'w'},
  {"max-line-length", no_argument, NULL, 'L'},
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
Print byte, word, and newline counts for each FILE, and a total line if\n\
more than one FILE is specified.  With no FILE, or when FILE is -,\n\
read standard input.\n\
  -c, --bytes            print the byte counts\n\
  -m, --chars            print the character counts\n\
  -l, --lines            print the newline counts\n\
"), stdout);
      fputs (_("\
  -L, --max-line-length  print the length of the longest line\n\
  -w, --words            print the word counts\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

static void
write_counts (uintmax_t lines,
	      uintmax_t words,
	      uintmax_t chars,
	      uintmax_t bytes,
	      uintmax_t linelength,
	      const char *file)
{
  char buf[INT_BUFSIZE_BOUND (uintmax_t)];
  char const *space = "";
  char const *format_int = (posixly_correct ? "%s" : "%7s");
  char const *format_sp_int = (posixly_correct ? "%s%s" : "%s%7s");

  if (print_lines)
    {
      printf (format_int, umaxtostr (lines, buf));
      space = " ";
    }
  if (print_words)
    {
      printf (format_sp_int, space, umaxtostr (words, buf));
      space = " ";
    }
  if (print_chars)
    {
      printf (format_sp_int, space, umaxtostr (chars, buf));
      space = " ";
    }
  if (print_bytes)
    {
      printf (format_sp_int, space, umaxtostr (bytes, buf));
      space = " ";
    }
  if (print_linelength)
    {
      printf (format_sp_int, space, umaxtostr (linelength, buf));
    }
  if (*file)
    printf (" %s", file);
  putchar ('\n');
}

static void
wc (int fd, const char *file)
{
  char buf[BUFFER_SIZE + 1];
  size_t bytes_read;
  uintmax_t lines, words, chars, bytes, linelength;
  int count_bytes, count_chars, count_complicated;

  lines = words = chars = bytes = linelength = 0;

  /* If in the current locale, chars are equivalent to bytes, we prefer
     counting bytes, because that's easier.  */
#if HAVE_MBRTOWC && (MB_LEN_MAX > 1)
  if (MB_CUR_MAX > 1)
    {
      count_bytes = print_bytes;
      count_chars = print_chars;
    }
  else
#endif
    {
      count_bytes = print_bytes + print_chars;
      count_chars = 0;
    }
  count_complicated = print_words + print_linelength;

  /* We need binary input, since `wc' relies on `lseek' and byte counts.  */
  SET_BINARY (fd);

  /* When counting only bytes, save some line- and word-counting
     overhead.  If FD is a `regular' Unix file, using lseek is enough
     to get its `size' in bytes.  Otherwise, read blocks of BUFFER_SIZE
     bytes at a time until EOF.  Note that the `size' (number of bytes)
     that wc reports is smaller than stats.st_size when the file is not
     positioned at its beginning.  That's why the lseek calls below are
     necessary.  For example the command
     `(dd ibs=99k skip=1 count=0; ./wc -c) < /etc/group'
     should make wc report `0' bytes.  */

  if (count_bytes && !count_chars && !print_lines && !count_complicated)
    {
      off_t current_pos, end_pos;
      struct stat stats;

      if (fstat (fd, &stats) == 0 && S_ISREG (stats.st_mode)
	  && (current_pos = lseek (fd, (off_t) 0, SEEK_CUR)) != -1
	  && (end_pos = lseek (fd, (off_t) 0, SEEK_END)) != -1)
	{
	  off_t diff;
	  /* Be careful here.  The current position may actually be
	     beyond the end of the file.  As in the example above.  */
	  bytes = (diff = end_pos - current_pos) < 0 ? 0 : diff;
	}
      else
	{
	  while ((bytes_read = safe_read (fd, buf, BUFFER_SIZE)) > 0)
	    {
	      if (bytes_read == SAFE_READ_ERROR)
		{
		  error (0, errno, "%s", file);
		  exit_status = 1;
		  break;
		}
	      bytes += bytes_read;
	    }
	}
    }
  else if (!count_chars && !count_complicated)
    {
      /* Use a separate loop when counting only lines or lines and bytes --
	 but not chars or words.  */
      while ((bytes_read = safe_read (fd, buf, BUFFER_SIZE)) > 0)
	{
	  register char *p = buf;

	  if (bytes_read == SAFE_READ_ERROR)
	    {
	      error (0, errno, "%s", file);
	      exit_status = 1;
	      break;
	    }

	  while ((p = memchr (p, '\n', (buf + bytes_read) - p)))
	    {
	      ++p;
	      ++lines;
	    }
	  bytes += bytes_read;
	}
    }
#if HAVE_MBRTOWC && (MB_LEN_MAX > 1)
# define SUPPORT_OLD_MBRTOWC 1
  else if (MB_CUR_MAX > 1)
    {
      int in_word = 0;
      uintmax_t linepos = 0;
      mbstate_t state;
      uintmax_t last_error_line = 0;
      int last_error_errno = 0;
# if SUPPORT_OLD_MBRTOWC
      /* Back-up the state before each multibyte character conversion and
	 move the last incomplete character of the buffer to the front
	 of the buffer.  This is needed because we don't know whether
	 the `mbrtowc' function updates the state when it returns -2, -
	 this is the ISO C 99 and glibc-2.2 behaviour - or not - amended
	 ANSI C, glibc-2.1 and Solaris 2.7 behaviour.  We don't have an
	 autoconf test for this, yet.  */
      size_t prev = 0; /* number of bytes carried over from previous round */
# else
      const size_t prev = 0;
# endif

      memset (&state, 0, sizeof (mbstate_t));
      while ((bytes_read = safe_read (fd, buf + prev, BUFFER_SIZE - prev)) > 0)
	{
	  const char *p;
# if SUPPORT_OLD_MBRTOWC
	  mbstate_t backup_state;
# endif
	  if (bytes_read == SAFE_READ_ERROR)
	    {
	      error (0, errno, "%s", file);
	      exit_status = 1;
	      break;
	    }

	  bytes += bytes_read;
	  p = buf;
	  bytes_read += prev;
	  do
	    {
	      wchar_t wide_char;
	      size_t n;

# if SUPPORT_OLD_MBRTOWC
	      backup_state = state;
# endif
	      n = mbrtowc (&wide_char, p, bytes_read, &state);
	      if (n == (size_t) -2)
		{
# if SUPPORT_OLD_MBRTOWC
		  state = backup_state;
# endif
		  break;
		}
	      if (n == (size_t) -1)
		{
		  /* Signal repeated errors only once per line.  */
		  if (!(lines + 1 == last_error_line
			&& errno == last_error_errno))
		    {
		      char line_number_buf[INT_BUFSIZE_BOUND (uintmax_t)];
		      last_error_line = lines + 1;
		      last_error_errno = errno;
		      error (0, errno, "%s:%s", file,
			     umaxtostr (last_error_line, line_number_buf));
		    }
		  p++;
		  bytes_read--;
		}
	      else
		{
		  if (n == 0)
		    {
		      wide_char = 0;
		      n = 1;
		    }
		  p += n;
		  bytes_read -= n;
		  chars++;
		  switch (wide_char)
		    {
		    case '\n':
		      lines++;
		      /* Fall through. */
		    case '\r':
		    case '\f':
		      if (linepos > linelength)
			linelength = linepos;
		      linepos = 0;
		      goto mb_word_separator;
		    case '\t':
		      linepos += 8 - (linepos % 8);
		      goto mb_word_separator;
		    case ' ':
		      linepos++;
		      /* Fall through. */
		    case '\v':
		    mb_word_separator:
		      if (in_word)
			{
			  in_word = 0;
			  words++;
			}
		      break;
		    default:
		      if (iswprint (wide_char))
			{
			  int width = wcwidth (wide_char);
			  if (width > 0)
			    linepos += width;
			  if (iswspace (wide_char))
			    goto mb_word_separator;
			  in_word = 1;
			}
		      break;
		    }
		}
	    }
	  while (bytes_read > 0);

# if SUPPORT_OLD_MBRTOWC
	  if (bytes_read > 0)
	    {
	      if (bytes_read == BUFFER_SIZE)
		{
		  /* Encountered a very long redundant shift sequence.  */
		  p++;
		  bytes_read--;
		}
	      memmove (buf, p, bytes_read);
	    }
	  prev = bytes_read;
# endif
	}
      if (linepos > linelength)
	linelength = linepos;
      if (in_word)
	words++;
    }
#endif
  else
    {
      int in_word = 0;
      uintmax_t linepos = 0;

      while ((bytes_read = safe_read (fd, buf, BUFFER_SIZE)) > 0)
	{
	  const char *p = buf;
	  if (bytes_read == SAFE_READ_ERROR)
	    {
	      error (0, errno, "%s", file);
	      exit_status = 1;
	      break;
	    }

	  bytes += bytes_read;
	  do
	    {
	      switch (*p++)
		{
		case '\n':
		  lines++;
		  /* Fall through. */
		case '\r':
		case '\f':
		  if (linepos > linelength)
		    linelength = linepos;
		  linepos = 0;
		  goto word_separator;
		case '\t':
		  linepos += 8 - (linepos % 8);
		  goto word_separator;
		case ' ':
		  linepos++;
		  /* Fall through. */
		case '\v':
		word_separator:
		  if (in_word)
		    {
		      in_word = 0;
		      words++;
		    }
		  break;
		default:
		  if (ISPRINT ((unsigned char) p[-1]))
		    {
		      linepos++;
		      if (ISSPACE ((unsigned char) p[-1]))
			goto word_separator;
		      in_word = 1;
		    }
		  break;
		}
	    }
	  while (--bytes_read);
	}
      if (linepos > linelength)
	linelength = linepos;
      if (in_word)
	words++;
    }

  if (count_chars < print_chars)
    chars = bytes;

  write_counts (lines, words, chars, bytes, linelength, file);
  total_lines += lines;
  total_words += words;
  total_chars += chars;
  total_bytes += bytes;
  if (linelength > max_line_length)
    max_line_length = linelength;
}

static void
wc_file (const char *file)
{
  if (STREQ (file, "-"))
    {
      have_read_stdin = 1;
      wc (0, file);
    }
  else
    {
      int fd = open (file, O_RDONLY);
      if (fd == -1)
	{
	  error (0, errno, "%s", file);
	  exit_status = 1;
	  return;
	}
      wc (fd, file);
      if (close (fd))
	{
	  error (0, errno, "%s", file);
	  exit_status = 1;
	}
    }
}

int
main (int argc, char **argv)
{
  int optc;
  int nfiles;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  exit_status = 0;
  posixly_correct = (getenv ("POSIXLY_CORRECT") != NULL);
  print_lines = print_words = print_chars = print_bytes = print_linelength = 0;
  total_lines = total_words = total_chars = total_bytes = max_line_length = 0;

  while ((optc = getopt_long (argc, argv, "clLmw", longopts, NULL)) != -1)
    switch (optc)
      {
      case 0:
	break;

      case 'c':
	print_bytes = 1;
	break;

      case 'm':
	print_chars = 1;
	break;

      case 'l':
	print_lines = 1;
	break;

      case 'w':
	print_words = 1;
	break;

      case 'L':
	print_linelength = 1;
	break;

      case_GETOPT_HELP_CHAR;

      case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

      default:
	usage (EXIT_FAILURE);
      }

  if (print_lines + print_words + print_chars + print_bytes + print_linelength
      == 0)
    print_lines = print_words = print_bytes = 1;

  nfiles = argc - optind;

  if (nfiles == 0)
    {
      have_read_stdin = 1;
      wc (0, "");
    }
  else
    {
      for (; optind < argc; ++optind)
	wc_file (argv[optind]);

      if (nfiles > 1)
	write_counts (total_lines, total_words, total_chars, total_bytes,
		      max_line_length, _("total"));
    }

  if (have_read_stdin && close (STDIN_FILENO) != 0)
    error (EXIT_FAILURE, errno, "-");

  exit (exit_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
