/* factor -- print factors of n.  lose if n > 2^32.
   Copyright (C) 86, 1995, 96 Free Software Foundation, Inc.

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

/* Written by Paul Rubin <phr@ocf.berkeley.edu>.
   Adapted for GNU, fixed to factor UINT_MAX by Jim Meyering.  */

#include <config.h>
#include <stdio.h>
#include <math.h>
#include <sys/types.h>

#include <assert.h>
#define NDEBUG 1

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif /* HAVE_LIMITS_H */

#ifndef UINT_MAX
# define UINT_MAX ((unsigned int) ~(unsigned int) 0)
#endif

#ifndef INT_MAX
# define INT_MAX ((int) (UINT_MAX >> 1))
#endif

#include "system.h"
#include "long-options.h"
#include "error.h"
#include "xstrtoul.h"
#include "readtokens.h"

/* Token delimiters when reading from a file.  */
#define DELIM "\n\t "

/* FIXME: if this program is ever modified to factor integers larger
   than 2^128, this constant (and the algorithm :-) will have to change.  */
#define MAX_N_FACTORS 128

/* The name this program was run with. */
char *program_name;

static void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [NUMBER]...\n\
  or:  %s OPTION\n\
"),
	      program_name, program_name);
      printf (_("\
Print factors of each NUMBER; read standard input with no arguments.\n\
\n\
  --help      display this help and exit\n\
  --version   output version information and exit\n\
\n\
  Print the prime factors of all specified integer NUMBERs.  If no arguments\n\
  are specified on the command line, they are read from standard input.\n\
"));
      puts (_("\nReport bugs to sh-utils-bugs@gnu.ai.mit.edu"));
    }
  exit (status);
}

/* FIXME: comment */

static int
factor (long unsigned int n0, int max_n_factors, long unsigned int *factors)
{
  register unsigned long n = n0, d;
  int n_factors = 0;
  unsigned int sqrt_n;

  if (n < 1)
    return n_factors;

  while (n % 2 == 0)
    {
      assert (n_factors < max_n_factors);
      factors[n_factors++] = 2;
      n /= 2;
    }

  /* The exit condition in the following loop is correct because
     any time it is tested one of these 3 conditions holds:
      (1) d divides n
      (2) n is prime
      (3) n is composite but has no factors less than d.
     If (1) or (2) obviously the right thing happens.
     If (3), then since n is composite it is >= d^2. */
  sqrt_n = (unsigned int) sqrt ((double) n);
  for (d = 3; d <= sqrt_n; d += 2)
    {
      while (n % d == 0)
	{
	  assert (n_factors < max_n_factors);
	  factors[n_factors++] = d;
	  n /= d;
	}
    }
  if (n != 1 || n0 == 1)
    {
      assert (n_factors < max_n_factors);
      factors[n_factors++] = n;
    }

  return n_factors;
}

/* FIXME: comment */

static int
print_factors (const char *s)
{
  unsigned long int factors[MAX_N_FACTORS];
  unsigned long n;
  int n_factors;
  int i;

  if (xstrtoul (s, NULL, 10, &n, "") != LONGINT_OK)
    {
      error (0, 0, _("`%s' is not a valid positive integer"), s);
      return 1;
    }
  n_factors = factor (n, MAX_N_FACTORS, factors);
  printf ("%lu:", n);
  for (i = 0; i < n_factors; i++)
    printf (" %lu", factors[i]);
  putchar ('\n');
  return 0;
}

static int
do_stdin (void)
{
  int fail = 0;
  token_buffer tokenbuffer;

  init_tokenbuffer (&tokenbuffer);

  for (;;)
    {
      long int token_length;

      token_length = readtoken (stdin, DELIM, sizeof (DELIM) - 1,
				&tokenbuffer);
      if (token_length < 0)
	break;
      fail |= print_factors (tokenbuffer.buffer);
    }
  free (tokenbuffer.buffer);

  return fail;
}

int
main (int argc, char **argv)
{
  int fail;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  parse_long_options (argc, argv, "factor", GNU_PACKAGE, VERSION, usage);

  fail = 0;
  if (argc == 1)
    fail = do_stdin ();
  else
    {
      int i;
      for (i = 1; i < argc; i++)
	fail |= print_factors (argv[i]);
    }
  if (fail)
    usage (1);

  exit (fail);
}
