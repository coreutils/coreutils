/* factor -- print factors of n.  lose if n > 2^32.
   Copyright (C) 1986 Free Software Foundation, Inc.

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

#include "system.h"
#include "version.h"
#include "long-options.h"
#include "error.h"

/* The name this program was run with. */
char *program_name;

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
Usage: %s [NUMBER]\n\
  or:  %s OPTION\n\
",
	      program_name, program_name);
      printf ("\
\n\
  --help      display this help and exit\n\
  --version   output version information and exit\n\
");
    }
  exit (status);
}

static int
factor (n0, max_n_factors, factors)
     unsigned long n0;
     int max_n_factors;
     unsigned long *factors;
{
  register unsigned long n = n0, d;
  int n_factors = 0;

  if (n < 1)
    return n_factors;

  while (n % 2 == 0)
    {
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
	  factors[n_factors++] = d;
	  n /= d;
	}
    }
  if (n != 1 || n0 == 1)
    factors[n_factors++] = n;

  return n_factors;
}

static void
print_factors (n)
     unsigned long int n;
{
  unsigned long int factors[64];
  int n_factors;
  int i;

  n_factors = factor (n, 64, factors);
  for (i=0; i<n_factors; i++)
    printf ("\t%lu\n", factors[i]);
}

static void
do_stdin ()
{
  char buf[1000];

  for (;;)
    {
      /* FIXME: Use getline.  */
      if (fgets (buf, sizeof buf, stdin) == 0)
	exit (0);
      /* FIXME: Use strtoul.  */
      print_factors ((unsigned long) atoi (buf));
    }
}

void
main (argc, argv)
     int argc;
     char **argv;
{
  program_name = argv[0];

  parse_long_options (argc, argv, "factor", version_string, usage);

  if (argc > 2)
    {
      error (0, 0, "too many arguments");
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
      fprintf (stderr, "Usage: %s [number]\n", argv[0]);
      exit (1);
    }
  exit (0);
}
