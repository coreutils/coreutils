/* factor -- print factors of n.  lose if n > 2^32.
   Copyright (C) 86, 1995 Free Software Foundation, Inc.

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

/* Written by Paul Rubin <phr@ocf.berkeley.edu>.  */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <assert.h>

#define NDEBUG 1

#include "system.h"
#include "version.h"
#include "long-options.h"
#include "error.h"
#include "readtokens.h"

/* Token delimiters when reading from a file.  */
#define DELIM "\n\t "

/* FIXME: if this program is ever modified to factor integers larger
   than 2^128, this (and the algorithm :-) will have to change.  */
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
Usage: %s [NUMBER]\n\
  or:  %s OPTION\n\
"),
	      program_name, program_name);
      printf (_("\
\n\
  --help      display this help and exit\n\
  --version   output version information and exit\n\
"));
    }
  exit (status);
}

static int
factor (long unsigned int n0, int max_n_factors, long unsigned int *factors)
{
  register unsigned long n = n0, d;
  int n_factors = 0;

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
  for (d = 3; d * d <= n; d += 2)
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

static void
print_factors (long unsigned int n)
{
  unsigned long int factors[MAX_N_FACTORS];
  int n_factors;
  int i;

  n_factors = factor (n, MAX_N_FACTORS, factors);
  for (i = 0; i < n_factors; i++)
    printf ("     %lu\n", factors[i]);
  putchar ('\n');
}

static void
do_stdin (void)
{
  token_buffer tokenbuffer;

  init_tokenbuffer (&tokenbuffer);

  for (;;)
    {
      long int token_length;

      token_length = readtoken (stdin, DELIM, sizeof (DELIM) - 1,
				&tokenbuffer);
      if (token_length < 0)
	break;
      /* FIXME: Use xstrtoul, not atoi.  */
      print_factors ((unsigned long) atoi (tokenbuffer.buffer));
    }
  free (tokenbuffer.buffer);
}

void
main (int argc, char **argv)
{
  program_name = argv[0];

  parse_long_options (argc, argv, "factor", version_string, usage);

  if (argc > 2)
    {
      error (0, 0, _("too many arguments"));
      usage (1);
    }

  if (argc == 1)
    do_stdin ();
  else if (argc == 2)
    {
      print_factors ((unsigned long) atoi (argv[1]));
    }
  else
    {
      fprintf (stderr, _("Usage: %s [number]\n"), argv[0]);
      exit (1);
    }
  exit (0);
}
