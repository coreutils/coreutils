/* wc - print the number of bytes, words, and lines in files
   Copyright (C) 85, 91, 1995-1999 Free Software Foundation, Inc.

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
#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include "system.h"
#include "error.h"
#include "human.h"
#include "safe-read.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "wc"

#define AUTHORS "Paul Rubin and David MacKenzie"

/* Size of atomic reads. */
#define BUFFER_SIZE (16 * 1024)

/* The name this program was run with. */
char *program_name;

/* Cumulative number of lines, words, and chars in all files so far.
   max_line_length is the maximum over all files processed so far.  */
static uintmax_t total_lines;
static uintmax_t total_words;
static uintmax_t total_chars;
static uintmax_t max_line_length;

/* Which counts to print. */
static int print_lines, print_words, print_chars, print_linelength;

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
  {"chars", no_argument, NULL, 'c'},
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
      printf (_("\
Print line, word, and byte counts for each FILE, and a total line if\n\
more than one FILE is specified.  With no FILE, or when FILE is -,\n\
read standard input.\n\
  -c, --bytes, --chars   print the byte counts\n\
  -l, --lines            print the newline counts\n\
  -L, --max-line-length  print the length of the longest line\n\
  -w, --words            print the word counts\n\
      --help             display this help and exit\n\
      --version          output version information and exit\n\
"));
      puts (_("\nReport bugs to <bug-textutils@gnu.org>."));
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

static void
write_counts (uintmax_t lines,
	      uintmax_t words,
	      uintmax_t chars,
	      uintmax_t linelength,
	      const char *file)
{
  char buf[LONGEST_HUMAN_READABLE + 1];
  char const *space = "";
  char const *format_int = (posixly_correct ? "%s" : "%7s");
  char const *format_sp_int = (posixly_correct ? "%s%s" : "%s%7s");

  if (print_lines)
    {
      printf (format_int, human_readable (lines, buf, 1, 1));
      space = " ";
    }
  if (print_words)
    {
      printf (format_sp_int, space, human_readable (words, buf, 1, 1));
      space = " ";
    }
  if (print_chars)
    {
      printf (format_sp_int, space, human_readable (chars, buf, 1, 1));
      space = " ";
    }
  if (print_linelength)
    {
      printf (format_sp_int, space, human_readable (linelength, buf, 1, 1));
    }
  if (*file)
    printf (" %s", file);
  putchar ('\n');
}

static void
wc (int fd, const char *file)
{
  char buf[BUFFER_SIZE + 1];
  ssize_t bytes_read;
  int in_word = 0;
  uintmax_t lines, words, chars, linelength;

  lines = words = chars = linelength = 0;

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

  if (print_chars && !print_words && !print_lines && !print_linelength)
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
	  chars = (diff = end_pos - current_pos) < 0 ? 0 : diff;
	}
      else
	{
	  while ((bytes_read = safe_read (fd, buf, BUFFER_SIZE)) > 0)
	    {
	      chars += bytes_read;
	    }
	  if (bytes_read < 0)
	    {
	      error (0, errno, "%s", file);
	      exit_status = 1;
	    }
	}
    }
  else if (!print_words && !print_linelength)
    {
      /* Use a separate loop when counting only lines or lines and bytes --
	 but not words.  */
      while ((bytes_read = safe_read (fd, buf, BUFFER_SIZE)) > 0)
	{
	  register char *p = buf;

	  while ((p = memchr (p, '\n', (buf + bytes_read) - p)))
	    {
	      ++p;
	      ++lines;
	    }
	  chars += bytes_read;
	}
      if (bytes_read < 0)
	{
	  error (0, errno, "%s", file);
	  exit_status = 1;
	}
    }
  else
    {
      uintmax_t linepos = 0;

      while ((bytes_read = safe_read (fd, buf, BUFFER_SIZE)) > 0)
	{
	  const char *p = buf;

	  chars += bytes_read;
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
		  linepos++;
		  in_word = 1;
		  break;
		}
	    }
	  while (--bytes_read);
	}
      if (bytes_read < 0)
	{
	  error (0, errno, "%s", file);
	  exit_status = 1;
	}
      if (linepos > linelength)
	linelength = linepos;
      if (in_word)
	words++;
    }

  write_counts (lines, words, chars, linelength, file);
  total_lines += lines;
  total_words += words;
  total_chars += chars;
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

  exit_status = 0;
  posixly_correct = (getenv ("POSIXLY_CORRECT") != NULL);
  print_lines = print_words = print_chars = print_linelength = 0;
  total_lines = total_words = total_chars = max_line_length = 0;

  while ((optc = getopt_long (argc, argv, "clLw", longopts, NULL)) != -1)
    switch (optc)
      {
      case 0:
	break;

      case 'c':
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
	usage (1);
      }

  if (print_lines + print_words + print_chars + print_linelength == 0)
    print_lines = print_words = print_chars = 1;

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
	write_counts (total_lines, total_words, total_chars, max_line_length,
		      _("total"));
    }

  if (have_read_stdin && close (0))
    error (EXIT_FAILURE, errno, "-");

  exit (exit_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
