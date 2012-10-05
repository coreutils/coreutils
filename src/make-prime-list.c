/* Factoring of uintmax_t numbers. Generation of needed tables.

   Contributed to the GNU project by Torbjörn Granlund and Niels Möller
   Contains code from GNU MP.

Copyright 2012 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see http://www.gnu.org/licenses/.  */

#include <config.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Deactivate config.h's "rpl_"-prefixed definitions of these symbols.  */
#undef fclose
#undef strerror

struct prime
{
  unsigned p;
  uintmax_t pinv; /* Inverse mod b = 2^{bitsize of uintmax_t} */
  uintmax_t lim; /* floor(UINTMAX_MAX / p) */
};

static uintmax_t _GL_ATTRIBUTE_CONST
binvert (uintmax_t a)
{
  uintmax_t x = 0xf5397db1 >> (4*((a/2) & 0x7));
  for (;;)
    {
      uintmax_t y = 2*x - x*x*a;
      if (y == x)
        return x;
      x = y;
    }
}

static void
process_prime (struct prime *info, unsigned p)
{
  info->p = p;
  info->pinv = binvert (p);
  info->lim = UINTMAX_MAX / (uintmax_t) p;
}

static void
output_primes (const struct prime *primes, unsigned nprimes)
{
  unsigned i;
  unsigned p;
  int is_prime;
  const char *suffix;

  puts ("/* Generated file -- DO NOT EDIT */\n");

  if (sizeof (uintmax_t) <= sizeof (unsigned long))
    suffix = "UL";
  else if (sizeof (uintmax_t) <= sizeof (unsigned long long))
    suffix = "ULL";
  else
    {
      fprintf (stderr, "Don't know how to write uintmax_t constants.\n");
      exit (EXIT_FAILURE);
    }

#define SZ (int)(2*sizeof (uintmax_t))

  for (i = 0, p = 2; i < nprimes; i++)
    {
      unsigned int d8 = i + 8 < nprimes ? primes[i + 8].p - primes[i].p : 0xff;
      if (255 < d8) /* this happens at 668221 */
        abort ();
      printf ("P (%2u, %3u, 0x%0*jx%s, 0x%0*jx%s) /* %d */\n",
              primes[i].p - p, d8,
              SZ, primes[i].pinv, suffix,
              SZ, primes[i].lim, suffix, primes[i].p);
      p = primes[i].p;
    }

  printf ("\n#undef FIRST_OMITTED_PRIME\n");

  /* Find next prime */
  do
    {
      p += 2;
      for (i = 0, is_prime = 1; is_prime; i++)
        {
          if (primes[i].p * primes[i].p > p)
            break;
          if ( (uintmax_t) p * primes[i].pinv <= primes[i].lim)
            {
              is_prime = 0;
              break;
            }
        }
    }
  while (!is_prime);

  printf ("#define FIRST_OMITTED_PRIME %d\n", p);
}

static void *
xalloc (size_t s)
{
  void *p = malloc (s);
  if (p)
    return p;

  fprintf (stderr, "Virtual memory exhausted.\n");
  exit (EXIT_FAILURE);
}

int
main (int argc, char **argv)
{
  int limit;

  char *sieve;
  size_t size, i;

  struct prime *prime_list;
  unsigned nprimes;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s LIMIT\n"
               "Produces a list of odd primes <= LIMIT\n", argv[0]);
      return EXIT_FAILURE;
    }
  limit = atoi (argv[1]);
  if (limit < 3)
    exit (EXIT_SUCCESS);

  /* Make limit odd */
  if ( !(limit & 1))
    limit--;

  size = (limit-1)/2;
  /* sieve[i] represents 3+2*i */
  sieve = xalloc (size);
  memset (sieve, 1, size);

  prime_list = xalloc (size * sizeof (*prime_list));
  nprimes = 0;

  for (i = 0; i < size;)
    {
      unsigned p = 3+2*i;
      unsigned j;

      process_prime (&prime_list[nprimes++], p);

      for (j = (p*p - 3)/2; j < size; j+= p)
        sieve[j] = 0;

      while (i < size && sieve[++i] == 0)
        ;
    }

  output_primes (prime_list, nprimes);

  if (ferror (stdout) + fclose (stdout))
    {
      fprintf (stderr, "write error: %s\n", strerror (errno));
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
