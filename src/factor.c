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

#include <stdio.h>

void do_stdin ();
void factor ();

void
main (argc, argv)
     int argc;
     char **argv;
{
  if (argc == 1)
    do_stdin ();
  else if (argc == 2)
    factor ((unsigned) atoi (argv[1]));
  else
    {
      fprintf (stderr, "Usage: %s [number]\n", argv[0]);
      exit (1);
    }
  exit (0);
}

void
factor (n0)
     unsigned long n0;
{
  register unsigned long n = n0, d;

  if (n < 1)
    return;

  while (n % 2 == 0)
    {
      printf ("\t2\n");
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
	  printf ("\t%d\n", d);
	  n /= d;
	}
    }
  if (n != 1 || n0 == 1)
    printf ("\t%d\n", n);
}

void
do_stdin ()
{
  char buf[1000];

  for (;;)
    {
      if (fgets (buf, sizeof buf, stdin) == 0)
	exit (0);
      factor ((unsigned long) atoi (buf));
    }
}
