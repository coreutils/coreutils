/* sum -- checksum and count the blocks in a file
   Copyright (C) 86, 89, 91, 1995-2002 Free Software Foundation, Inc.

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

/* Like BSD sum or SysV sum -r, except like SysV sum if -s option is given. */

/* Written by Kayvan Aghaiepour and David MacKenzie. */

#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include "system.h"
#include "closeout.h"
#include "error.h"
#include "human.h"
#include "safe-read.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "sum"

#define AUTHORS N_ ("Kayvan Aghaiepour and David MacKenzie")

/* The name this program was run with. */
char *program_name;

/* Nonzero if any of the files read were the standard input. */
static int have_read_stdin;

static struct option const longopts[] =
{
  {"sysv", no_argument, NULL, 's'},
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
Print checksum and block counts for each FILE.\n\
\n\
  -r              defeat -s, use BSD sum algorithm, use 1K blocks\n\
  -s, --sysv      use System V sum algorithm, use 512 bytes blocks\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
With no FILE, or when FILE is -, read standard input.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Calculate and print the rotated checksum and the size in 1K blocks
   of file FILE, or of the standard input if FILE is "-".
   If PRINT_NAME is >1, print FILE next to the checksum and size.
   The checksum varies depending on sizeof(int).
   Return 0 if successful, -1 if an error occurs. */

static int
bsd_sum_file (const char *file, int print_name)
{
  register FILE *fp;
  register int checksum = 0;	/* The checksum mod 2^16. */
  register uintmax_t total_bytes = 0;	/* The number of bytes. */
  register int ch;		/* Each character read. */
  char hbuf[LONGEST_HUMAN_READABLE + 1];

  if (STREQ (file, "-"))
    {
      fp = stdin;
      have_read_stdin = 1;
    }
  else
    {
      fp = fopen (file, "r");
      if (fp == NULL)
	{
	  error (0, errno, "%s", file);
	  return -1;
	}
    }
  /* Need binary I/O, or else byte counts and checksums are incorrect.  */
  SET_BINARY (fileno(fp));

  while ((ch = getc (fp)) != EOF)
    {
      total_bytes++;
      checksum = (checksum >> 1) + ((checksum & 1) << 15);
      checksum += ch;
      checksum &= 0xffff;	/* Keep it within bounds. */
    }

  if (ferror (fp))
    {
      error (0, errno, "%s", file);
      if (!STREQ (file, "-"))
	fclose (fp);
      return -1;
    }

  if (!STREQ (file, "-") && fclose (fp) == EOF)
    {
      error (0, errno, "%s", file);
      return -1;
    }

  printf ("%05d %5s", checksum,
	  human_readable (total_bytes, hbuf, human_ceiling, 1, 1024));
  if (print_name > 1)
    printf (" %s", file);
  putchar ('\n');

  return 0;
}

/* Calculate and print the checksum and the size in 512-byte blocks
   of file FILE, or of the standard input if FILE is "-".
   If PRINT_NAME is >0, print FILE next to the checksum and size.
   Return 0 if successful, -1 if an error occurs. */

static int
sysv_sum_file (const char *file, int print_name)
{
  int fd;
  unsigned char buf[8192];
  uintmax_t total_bytes = 0;
  char hbuf[LONGEST_HUMAN_READABLE + 1];
  int r;
  int checksum;

  /* The sum of all the input bytes, modulo (UINT_MAX + 1).  */
  unsigned int s = 0;

  if (STREQ (file, "-"))
    {
      fd = 0;
      have_read_stdin = 1;
    }
  else
    {
      fd = open (file, O_RDONLY);
      if (fd == -1)
	{
	  error (0, errno, "%s", file);
	  return -1;
	}
    }
  /* Need binary I/O, or else byte counts and checksums are incorrect.  */
  SET_BINARY (fd);

  while (1)
    {
      size_t i;
      size_t bytes_read = safe_read (fd, buf, sizeof buf);

      if (bytes_read == 0)
	break;

      if (bytes_read == SAFE_READ_ERROR)
	{
	  error (0, errno, "%s", file);
	  if (!STREQ (file, "-"))
	    close (fd);
	  return -1;
	}

      for (i = 0; i < bytes_read; i++)
	s += buf[i];
      total_bytes += bytes_read;
    }

  if (!STREQ (file, "-") && close (fd) == -1)
    {
      error (0, errno, "%s", file);
      return -1;
    }

  r = (s & 0xffff) + ((s & 0xffffffff) >> 16);
  checksum = (r & 0xffff) + (r >> 16);

  printf ("%d %s", checksum,
	  human_readable (total_bytes, hbuf, human_ceiling, 1, 512));
  if (print_name)
    printf (" %s", file);
  putchar ('\n');

  return 0;
}

int
main (int argc, char **argv)
{
  int errors = 0;
  int optc;
  int files_given;
  int (*sum_func) (const char *, int) = bsd_sum_file;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  have_read_stdin = 0;

  while ((optc = getopt_long (argc, argv, "rs", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;

	case 'r':		/* For SysV compatibility. */
	  sum_func = bsd_sum_file;
	  break;

	case 's':
	  sum_func = sysv_sum_file;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  files_given = argc - optind;
  if (files_given == 0)
    {
      if ((*sum_func) ("-", files_given) < 0)
	errors = 1;
    }
  else
    for (; optind < argc; optind++)
      if ((*sum_func) (argv[optind], files_given) < 0)
	errors = 1;

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, "-");
  exit (errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
