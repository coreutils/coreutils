/* factor -- print prime factors of n.
   Copyright (C) 86, 1995-2002 Free Software Foundation, Inc.

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
#include <sys/types.h>

#include <assert.h>
#define NDEBUG 1

#include "system.h"
#include "closeout.h"
#include "error.h"
#include "inttostr.h"
#include "long-options.h"
#include "readtokens.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "factor"

#define AUTHORS "Paul Rubin"

/* Token delimiters when reading from a file.  */
#define DELIM "\n\t "

/* FIXME: if this program is ever modified to factor integers larger
   than 2^128, this constant (and the algorithm :-) will have to change.  */
#define MAX_N_FACTORS 128

/* The trial divisor increment wheel.  Use it to skip over divisors that
   are composites of 2, 3, 5, 7, or 11.  The part from WHEEL_START up to
   WHEEL_END is reused periodically, while the "lead in" is used to test
   for those primes and to jump onto the wheel.  For more information, see
   http://www.utm.edu/research/primes/glossary/WheelFactorization.html  */

#include "wheel-size.h"  /* For the definition of WHEEL_SIZE.  */
static const unsigned int wheel_tab[] =
  {
#include "wheel.h"
  };

#define WHEEL_START (wheel_tab + WHEEL_SIZE)
#define WHEEL_END (wheel_tab + (sizeof wheel_tab / sizeof wheel_tab[0]))

/* The name this program was run with. */
char *program_name;

void
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
      fputs (_("\
Print the prime factors of each NUMBER.\n\
\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
  Print the prime factors of all specified integer NUMBERs.  If no arguments\n\
  are specified on the command line, they are read from standard input.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

/* FIXME: comment */

static int
factor (uintmax_t n0, int max_n_factors, uintmax_t *factors)
{
  register uintmax_t n = n0, d, q;
  int n_factors = 0;
  unsigned int const *w = wheel_tab;

  if (n < 1)
    return n_factors;

  /* The exit condition in the following loop is correct because
     any time it is tested one of these 3 conditions holds:
      (1) d divides n
      (2) n is prime
      (3) n is composite but has no factors less than d.
     If (1) or (2) obviously the right thing happens.
     If (3), then since n is composite it is >= d^2. */

  d = 2;
  do
    {
      q = n / d;
      while (n == q * d)
	{
	  assert (n_factors < max_n_factors);
	  factors[n_factors++] = d;
	  n = q;
	  q = n / d;
	}
      d += *(w++);
      if (w == WHEEL_END)
	w = WHEEL_START;
    }
  while (d <= q);

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
  uintmax_t factors[MAX_N_FACTORS];
  uintmax_t n;
  int n_factors;
  int i;
  char buf[INT_BUFSIZE_BOUND (uintmax_t)];

  if (xstrtoumax (s, NULL, 10, &n, "") != LONGINT_OK)
    {
      error (0, 0, _("`%s' is not a valid positive integer"), s);
      return 1;
    }
  n_factors = factor (n, MAX_N_FACTORS, factors);
  printf ("%s:", umaxtostr (n, buf));
  for (i = 0; i < n_factors; i++)
    printf (" %s", umaxtostr (factors[i], buf));
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

  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      AUTHORS, usage);
  /* The above handles --help and --version.
     Since there is no other invocation of getopt, handle `--' here.  */
  if (argc > 1 && STREQ (argv[1], "--"))
    {
      --argc;
      ++argv;
    }

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
    usage (EXIT_FAILURE);

  exit (fail);
}
