/* wc - print the number of bytes, words, and lines in files
   Copyright (C) 1985, 1991 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Written by Paul Rubin, phr@ocf.berkeley.edu
   and David MacKenzie, djm@gnu.ai.mit.edu. */

#ifdef HAVE_CONFIG_H
#if defined (CONFIG_BROKETS)
/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because it found this file in $srcdir).  */
#include <config.h>
#else
#include "config.h"
#endif
#endif

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include "system.h"
#include "version.h"

/* Size of atomic reads. */
#define BUFFER_SIZE (16 * 1024)

void error ();

static void wc ();
static void wc_file ();
static void write_counts ();

/* The name this program was run with. */
char *program_name;

/* Cumulative number of lines, words, and chars in all files so far. */
static unsigned long total_lines, total_words, total_chars;

/* Which counts to print. */
static int print_lines, print_words, print_chars;

/* Nonzero if we have ever read the standard input. */
static int have_read_stdin;

/* The error code to return to the system. */
static int exit_status;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output then exits.  */
static int show_version;

static struct option const longopts[] =
{
  {"bytes", no_argument, NULL, 'c'},
  {"chars", no_argument, NULL, 'c'},
  {"lines", no_argument, NULL, 'l'},
  {"words", no_argument, NULL, 'w'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("\
Usage: %s [OPTION]... [FILE]...\n\
",
	      program_name);
      printf ("\
\n\
  -c, --bytes, --chars   print the byte counts\n\
  -l, --lines            print the newline counts\n\
  -w, --words            print the word counts\n\
      --help             display this help and exit\n\
      --version          output version information and exit\n\
\n\
Print lines, words and bytes in that order.  If none of -clw, select\n\
them all.  With no FILE, or when FILE is -, read standard input.\n\
");
    }
  exit (status);
}

void
main (argc, argv)
     int argc;
     char **argv;
{
  int optc;
  int nfiles;

  program_name = argv[0];
  exit_status = 0;
  print_lines = print_words = print_chars = 0;
  total_lines = total_words = total_chars = 0;

  while ((optc = getopt_long (argc, argv, "clw", longopts, (int *) 0)) != EOF)
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

      default:
	usage (1);
      }

  if (show_version)
    {
      printf ("%s\n", version_string);
      exit (0);
    }

  if (show_help)
    usage (0);

  if (print_lines + print_words + print_chars == 0)
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
	write_counts (total_lines, total_words, total_chars, "total");
    }

  if (have_read_stdin && close (0))
    error (1, errno, "-");

  exit (exit_status);
}

static void
wc_file (file)
     char *file;
{
  if (!strcmp (file, "-"))
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

static void
wc (fd, file)
     int fd;
     char *file;
{
  char buf[BUFFER_SIZE];
  register int bytes_read;
  register int in_word = 0;
  register unsigned long lines, words, chars;
  struct stat stats;

  lines = words = chars = 0;

  if (print_chars && !print_words && !print_lines
      && fstat (fd, &stats) == 0 && S_ISREG (stats.st_mode))
    {
      chars = stats.st_size;
    }
  else
    {
      while ((bytes_read = read (fd, buf, BUFFER_SIZE)) > 0)
	{
	  register char *p = buf;

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
		case '\t':
		case '\v':
		case ' ':
		  if (in_word)
		    {
		      in_word = 0;
		      words++;
		    }
		  break;
		default:
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
      if (in_word)
	words++;
    }

  write_counts (lines, words, chars, file);
  total_lines += lines;
  total_words += words;
  total_chars += chars;
}

static void
write_counts (lines, words, chars, file)
     unsigned long lines, words, chars;
     char *file;
{
  if (print_lines)
    printf ("%7lu", lines);
  if (print_words)
    {
      if (print_lines)
	putchar (' ');
      printf ("%7lu", words);
    }
  if (print_chars)
    {
      if (print_lines || print_words)
	putchar (' ');
      printf ("%7lu", chars);
    }
  if (*file)
    printf (" %s", file);
  putchar ('\n');
}
