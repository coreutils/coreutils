/* head -- output first part of file(s)
   Copyright (C) 89, 90, 91, 95, 96, 1997 Free Software Foundation, Inc.

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

/* Options: (see usage)
   Reads from standard input if no files are given or when a filename of
   ``-'' is encountered.
   By default, filename headers are printed only if more than one file
   is given.
   By default, prints the first 10 lines (head -n 10).

   David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include "system.h"
#include "error.h"

/* Number of lines/chars/blocks to head. */
#define DEFAULT_NUMBER 10

/* Size of atomic reads. */
#define BUFSIZE (512 * 8)

/* Number of bytes per item we are printing.
   If 0, head in lines. */
static int unit_size;

/* If nonzero, print filename headers. */
static int print_headers;

/* When to print the filename banners. */
enum header_mode
{
  multiple_files, always, never
};

int safe_read ();

/* The name this program was run with. */
char *program_name;

/* Have we ever read standard input?  */
static int have_read_stdin;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output then exit.  */
static int show_version;

static struct option const long_options[] =
{
  {"bytes", required_argument, NULL, 'c'},
  {"lines", required_argument, NULL, 'n'},
  {"quiet", no_argument, NULL, 'q'},
  {"silent", no_argument, NULL, 'q'},
  {"verbose", no_argument, NULL, 'v'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

static void
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
Print first 10 lines of each FILE to standard output.\n\
With more than one FILE, precede each with a header giving the file name.\n\
With no FILE, or when FILE is -, read standard input.\n\
\n\
  -c, --bytes=SIZE         print first SIZE bytes\n\
  -n, --lines=NUMBER       print first NUMBER lines instead of first 10\n\
  -q, --quiet, --silent    never print headers giving file names\n\
  -v, --verbose            always print headers giving file names\n\
      --help               display this help and exit\n\
      --version            output version information and exit\n\
\n\
SIZE may have a multiplier suffix: b for 512, k for 1K, m for 1 Meg.\n\
If -VALUE is used as first OPTION, read -c VALUE when one of\n\
multipliers bkm follows concatenated, else read -n VALUE.\n\
"));
      puts (_("\nReport bugs to <textutils-bugs@gnu.ai.mit.edu>."));
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Convert STR, a string of ASCII digits, into an unsigned integer.
   Return -1 if STR does not represent a valid unsigned integer. */

static long
atou (const char *str)
{
  int value;

  for (value = 0; ISDIGIT (*str); ++str)
    value = value * 10 + *str - '0';
  return *str ? -1 : value;
}

static void
parse_unit (char *str)
{
  int arglen = strlen (str);

  if (arglen == 0)
    return;

  switch (str[arglen - 1])
    {
    case 'b':
      unit_size = 512;
      str[arglen - 1] = '\0';
      break;
    case 'k':
      unit_size = 1024;
      str[arglen - 1] = '\0';
      break;
    case 'm':
      unit_size = 1048576;
      str[arglen - 1] = '\0';
      break;
    }
}

static void
write_header (const char *filename)
{
  static int first_file = 1;

  printf ("%s==> %s <==\n", (first_file ? "" : "\n"), filename);
  first_file = 0;
}

static int
head_bytes (const char *filename, int fd, long int bytes_to_write)
{
  char buffer[BUFSIZE];
  int bytes_read;

  while (bytes_to_write)
    {
      bytes_read = safe_read (fd, buffer, BUFSIZE);
      if (bytes_read < 0)
	{
	  error (0, errno, "%s", filename);
	  return 1;
	}
      if (bytes_read == 0)
	break;
      if (bytes_read > bytes_to_write)
	bytes_read = bytes_to_write;
      if (fwrite (buffer, 1, bytes_read, stdout) == 0)
	error (EXIT_FAILURE, errno, _("write error"));
      bytes_to_write -= bytes_read;
    }
  return 0;
}

static int
head_lines (const char *filename, int fd, long int lines_to_write)
{
  char buffer[BUFSIZE];
  int bytes_read;
  int bytes_to_write;

  while (lines_to_write)
    {
      bytes_read = safe_read (fd, buffer, BUFSIZE);
      if (bytes_read < 0)
	{
	  error (0, errno, "%s", filename);
	  return 1;
	}
      if (bytes_read == 0)
	break;
      bytes_to_write = 0;
      while (bytes_to_write < bytes_read)
	if (buffer[bytes_to_write++] == '\n' && --lines_to_write == 0)
	  break;
      if (fwrite (buffer, 1, bytes_to_write, stdout) == 0)
	error (EXIT_FAILURE, errno, _("write error"));
    }
  return 0;
}

static int
head (const char *filename, int fd, long int number)
{
  if (unit_size)
    return head_bytes (filename, fd, number);
  else
    return head_lines (filename, fd, number);
}

static int
head_file (const char *filename, long int number)
{
  int fd;

  if (!strcmp (filename, "-"))
    {
      have_read_stdin = 1;
      filename = _("standard input");
      if (print_headers)
	write_header (filename);
      return head (filename, 0, number);
    }
  else
    {
      fd = open (filename, O_RDONLY);
      if (fd >= 0)
	{
	  int errors;

	  if (print_headers)
	    write_header (filename);
	  errors = head (filename, fd, number);
	  if (close (fd) == 0)
	    return errors;
	}
      error (0, errno, "%s", filename);
      return 1;
    }
}

int
main (int argc, char **argv)
{
  enum header_mode header_mode = multiple_files;
  int exit_status = 0;
  long number = -1;		/* Number of items to print (-1 if undef.). */
  int c;			/* Option character. */

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  have_read_stdin = 0;
  unit_size = 0;
  print_headers = 0;

  if (argc > 1 && argv[1][0] == '-' && ISDIGIT (argv[1][1]))
    {
      /* Old option syntax; a dash, one or more digits, and one or
	 more option letters.  Move past the number. */
      for (number = 0, ++argv[1]; ISDIGIT (*argv[1]); ++argv[1])
	number = number * 10 + *argv[1] - '0';
      /* Parse any appended option letters. */
      while (*argv[1])
	{
	  switch (*argv[1])
	    {
	    case 'b':
	      unit_size = 512;
	      break;

	    case 'c':
	      unit_size = 1;
	      break;

	    case 'k':
	      unit_size = 1024;
	      break;

	    case 'l':
	      unit_size = 0;
	      break;

	    case 'm':
	      unit_size = 1048576;
	      break;

	    case 'q':
	      header_mode = never;
	      break;

	    case 'v':
	      header_mode = always;
	      break;

	    default:
	      error (0, 0, _("unrecognized option `-%c'"), *argv[1]);
	      usage (1);
	    }
	  ++argv[1];
	}
      /* Make the options we just parsed invisible to getopt. */
      argv[1] = argv[0];
      argv++;
      argc--;
    }

  while ((c = getopt_long (argc, argv, "c:n:qv", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case 'c':
	  unit_size = 1;
	  parse_unit (optarg);
	  goto getnum;
	case 'n':
	  unit_size = 0;
	getnum:
	  /* FIXME: use xstrtoul instead.  */
	  number = atou (optarg);
	  if (number == -1)
	    error (EXIT_FAILURE, 0, _("invalid number `%s'"), optarg);
	  break;

	case 'q':
	  header_mode = never;
	  break;

	case 'v':
	  header_mode = always;
	  break;

	default:
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("head (%s) %s\n", GNU_PACKAGE, VERSION);
      exit (EXIT_SUCCESS);
    }

  if (show_help)
    usage (0);

  if (number == -1)
    number = DEFAULT_NUMBER;

  if (unit_size > 1)
    number *= unit_size;

  if (header_mode == always
      || (header_mode == multiple_files && optind < argc - 1))
    print_headers = 1;

  if (optind == argc)
    exit_status |= head_file ("-", number);

  for (; optind < argc; ++optind)
    exit_status |= head_file (argv[optind], number);

  if (have_read_stdin && close (0) < 0)
    error (EXIT_FAILURE, errno, "-");
  if (fclose (stdout) == EOF)
    error (EXIT_FAILURE, errno, _("write error"));

  exit (exit_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
