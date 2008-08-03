/* factor -- print prime factors of n.
   Copyright (C) 86, 1995-2005, 2007-2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Paul Rubin <phr@ocf.berkeley.edu>.
   Adapted for GNU, fixed to factor UINT_MAX by Jim Meyering.
   Arbitrary-precision code adapted by James Youngman from Torbjörn
   Granlund's factorize.c, from GNU MP version 4.2.2.
*/

#include <config.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#if HAVE_GMP
#include <gmp.h>
#endif


#include <assert.h>
#define NDEBUG 1

#include "system.h"
#include "error.h"
#include "quote.h"
#include "readtokens.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "factor"

#define AUTHORS proper_name ("Paul Rubin")

/* Token delimiters when reading from a file.  */
#define DELIM "\n\t "

static bool verbose = false;

typedef enum
  {
    ALGO_AUTOSELECT,		/* default */
    ALGO_GMP,			/* --bignum */
    ALGO_SINGLE			/* --no-bignum */
  } ALGORITHM_CHOICE;
static ALGORITHM_CHOICE algorithm = ALGO_AUTOSELECT;


#if HAVE_GMP
static mpz_t *factor = NULL;
static size_t nfactors_found = 0;
static size_t nfactors_allocated = 0;

static void
debug (char const *fmt, ...)
{
  if (verbose)
    {
      va_list ap;
      va_start (ap, fmt);
      vfprintf (stderr, fmt, ap);
    }
}

static void
emit_factor (mpz_t n)
{
  if (nfactors_found == nfactors_allocated)
    factor = x2nrealloc (factor, &nfactors_allocated, sizeof *factor);
  mpz_init (factor[nfactors_found]);
  mpz_set (factor[nfactors_found], n);
  ++nfactors_found;
}

static void
emit_ul_factor (unsigned long int i)
{
  mpz_t t;
  mpz_init (t);
  mpz_set_ui (t, i);
  emit_factor (t);
}

static void
factor_using_division (mpz_t t, unsigned int limit)
{
  mpz_t q, r;
  unsigned long int f;
  int ai;
  static unsigned int const add[] = {4, 2, 4, 2, 4, 6, 2, 6};
  unsigned int const *addv = add;
  unsigned int failures;

  debug ("[trial division (%u)] ", limit);

  mpz_init (q);
  mpz_init (r);

  f = mpz_scan1 (t, 0);
  mpz_div_2exp (t, t, f);
  while (f)
    {
      emit_ul_factor (2);
      --f;
    }

  for (;;)
    {
      mpz_tdiv_qr_ui (q, r, t, 3);
      if (mpz_cmp_ui (r, 0) != 0)
	break;
      mpz_set (t, q);
      emit_ul_factor (3);
    }

  for (;;)
    {
      mpz_tdiv_qr_ui (q, r, t, 5);
      if (mpz_cmp_ui (r, 0) != 0)
	break;
      mpz_set (t, q);
      emit_ul_factor (5);
    }

  failures = 0;
  f = 7;
  ai = 0;
  while (mpz_cmp_ui (t, 1) != 0)
    {
      mpz_tdiv_qr_ui (q, r, t, f);
      if (mpz_cmp_ui (r, 0) != 0)
	{
	  f += addv[ai];
	  if (mpz_cmp_ui (q, f) < 0)
	    break;
	  ai = (ai + 1) & 7;
	  failures++;
	  if (failures > limit)
	    break;
	}
      else
	{
	  mpz_swap (t, q);
	  emit_ul_factor (f);
	  failures = 0;
	}
    }

  mpz_clear (q);
  mpz_clear (r);
}

static void
factor_using_pollard_rho (mpz_t n, int a_int)
{
  mpz_t x, x1, y, P;
  mpz_t a;
  mpz_t g;
  mpz_t t1, t2;
  int k, l, c, i;

  debug ("[pollard-rho (%d)] ", a_int);

  mpz_init (g);
  mpz_init (t1);
  mpz_init (t2);

  mpz_init_set_si (a, a_int);
  mpz_init_set_si (y, 2);
  mpz_init_set_si (x, 2);
  mpz_init_set_si (x1, 2);
  k = 1;
  l = 1;
  mpz_init_set_ui (P, 1);
  c = 0;

  while (mpz_cmp_ui (n, 1) != 0)
    {
S2:
      mpz_mul (x, x, x); mpz_add (x, x, a); mpz_mod (x, x, n);

      mpz_sub (t1, x1, x); mpz_mul (t2, P, t1); mpz_mod (P, t2, n);
      c++;
      if (c == 20)
	{
	  c = 0;
	  mpz_gcd (g, P, n);
	  if (mpz_cmp_ui (g, 1) != 0)
	    goto S4;
	  mpz_set (y, x);
	}

      k--;
      if (k > 0)
	goto S2;

      mpz_gcd (g, P, n);
      if (mpz_cmp_ui (g, 1) != 0)
	goto S4;

      mpz_set (x1, x);
      k = l;
      l = 2 * l;
      for (i = 0; i < k; i++)
	{
	  mpz_mul (x, x, x); mpz_add (x, x, a); mpz_mod (x, x, n);
	}
      mpz_set (y, x);
      c = 0;
      goto S2;
S4:
      do
	{
	  mpz_mul (y, y, y); mpz_add (y, y, a); mpz_mod (y, y, n);
	  mpz_sub (t1, x1, y); mpz_gcd (g, t1, n);
	}
      while (mpz_cmp_ui (g, 1) == 0);

      mpz_div (n, n, g);	/* divide by g, before g is overwritten */

      if (!mpz_probab_prime_p (g, 3))
	{
	  do
	    {
	      mp_limb_t a_limb;
	      mpn_random (&a_limb, (mp_size_t) 1);
	      a_int = (int) a_limb;
	    }
	  while (a_int == -2 || a_int == 0);

	  debug ("[composite factor--restarting pollard-rho] ");
	  factor_using_pollard_rho (g, a_int);
	}
      else
	{
	  emit_factor (g);
	}
      mpz_mod (x, x, n);
      mpz_mod (x1, x1, n);
      mpz_mod (y, y, n);
      if (mpz_probab_prime_p (n, 3))
	{
	  emit_factor (n);
	  break;
	}
    }

  mpz_clear (g);
  mpz_clear (P);
  mpz_clear (t2);
  mpz_clear (t1);
  mpz_clear (a);
  mpz_clear (x1);
  mpz_clear (x);
  mpz_clear (y);
}

/* Arbitrary-precision factoring */
static bool
extract_factors_multi (char const *s)
{
  unsigned int division_limit;
  size_t n_bits;
  mpz_t t;

  mpz_init (t);
  if (1 != gmp_sscanf (s, "%Zd", t))
    {
      error (0, 0, _("%s is not a valid positive integer"), quote (s));
      return false;
    }

  printf ("%s:", s);

  if (mpz_sgn (t) == 0)
    {
      mpz_clear (t);
      return true;		/* 0; no factors */
    }

  /* Set the trial division limit according to the size of t.  */
  n_bits = mpz_sizeinbase (t, 2);
  division_limit = MIN (n_bits, 1000);
  division_limit *= division_limit;

  factor_using_division (t, division_limit);

  if (mpz_cmp_ui (t, 1) != 0)
    {
      debug ("[is number prime?] ");
      if (mpz_probab_prime_p (t, 3))
	{
	  emit_factor (t);
	}
      else
	{
	  factor_using_pollard_rho (t, 1);
	}
    }
  mpz_clear (t);
  return true;
}
#endif

/* The maximum number of factors, including -1, for negative numbers.  */
#define MAX_N_FACTORS (sizeof (uintmax_t) * CHAR_BIT)

/* The trial divisor increment wheel.  Use it to skip over divisors that
   are composites of 2, 3, 5, 7, or 11.  The part from WHEEL_START up to
   WHEEL_END is reused periodically, while the "lead in" is used to test
   for those primes and to jump onto the wheel.  For more information, see
   http://www.utm.edu/research/primes/glossary/WheelFactorization.html  */

#include "wheel-size.h"  /* For the definition of WHEEL_SIZE.  */
static const unsigned char wheel_tab[] =
  {
#include "wheel.h"
  };

#define WHEEL_START (wheel_tab + WHEEL_SIZE)
#define WHEEL_END (wheel_tab + (sizeof wheel_tab / sizeof wheel_tab[0]))

/* FIXME: comment */

static size_t
factor_wheel (uintmax_t n0, size_t max_n_factors, uintmax_t *factors)
{
  uintmax_t n = n0, d, q;
  size_t n_factors = 0;
  unsigned char const *w = wheel_tab;

  if (n <= 1)
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

/* Single-precision factoring */
static bool
print_factors_single (char const *s)
{
  uintmax_t factors[MAX_N_FACTORS];
  uintmax_t n;
  size_t n_factors;
  size_t i;
  char buf[INT_BUFSIZE_BOUND (uintmax_t)];
  strtol_error err;

  if ((err = xstrtoumax (s, NULL, 10, &n, "")) != LONGINT_OK)
    {
      if (err == LONGINT_OVERFLOW)
	error (0, 0, _("%s is too large"), quote (s));
      else
	error (0, 0, _("%s is not a valid positive integer"), quote (s));
      return false;
    }
  n_factors = factor_wheel (n, MAX_N_FACTORS, factors);
  printf ("%s:", umaxtostr (n, buf));
  for (i = 0; i < n_factors; i++)
    printf (" %s", umaxtostr (factors[i], buf));
  putchar ('\n');
  return true;
}

#if HAVE_GMP
static int
mpcompare (const void *av, const void *bv)
{
  mpz_t *const *a = av;
  mpz_t *const *b = bv;
  return mpz_cmp (**a, **b);
}

static void
sort_and_print_factors (void)
{
  mpz_t **faclist;
  size_t i;

  faclist = xcalloc (nfactors_found, sizeof *faclist);
  for (i = 0; i < nfactors_found; ++i)
    {
      faclist[i] = &factor[i];
    }
  qsort (faclist, nfactors_found, sizeof *faclist, mpcompare);

  for (i = 0; i < nfactors_found; ++i)
    {
      fputc (' ', stdout);
      mpz_out_str (stdout, 10, *faclist[i]);
    }
  putchar ('\n');
  free (faclist);
}

static void
free_factors (void)
{
  size_t i;

  for (i = 0; i < nfactors_found; ++i)
    {
      mpz_clear (factor[i]);
    }
  /* Don't actually free factor[] because in the case where we are
     reading numbers from stdin, we may be about to use it again.  */
  nfactors_found = 0;
}


/* Emit the factors of the indicated number.  If we have the option of using
   either algorithm, we select on the basis of the length of the number.
   For longer numbers, we prefer the MP algorithm even if the native algorithm
   has enough digits, because the algorithm is better.   The turnover point
   depends on the value as well as the length, but since we don't already know
   if the number presented has small factors, we just switch over at 6 digits.
*/
static bool
print_factors (char const *s)
{
  if (ALGO_AUTOSELECT == algorithm)
    {
      const size_t digits = strlen (s); /* upper limit on number of digits */
      algorithm = digits < 6 ? ALGO_SINGLE : ALGO_GMP;
    }
  if (ALGO_GMP == algorithm)
    {
      debug ("[%s]", _("using arbitrary-precision arithmetic"));
      if (extract_factors_multi (s))
	{
	  sort_and_print_factors ();
	  free_factors ();
	  return true;
	}
      else
	{
	  return false;
	}
    }
  else
    {
      debug ("[%s]", _("using single-precision arithmetic"));
      return print_factors_single (s);
    }
}
#else
static bool
print_factors (const char *s)	/* Single-precision only. */
{
  if (ALGO_GMP == algorithm)
    {
      error (0, 0, _("arbitrary-precision arithmetic is not available"));
      return false;
    }
  return print_factors_single (s);
}
#endif

enum
{
  VERBOSE_OPTION = CHAR_MAX + 1,
  USE_BIGNUM,
  NO_USE_BIGNUM
};

static struct option const long_options[] =
{
  {"verbose", no_argument, NULL, VERBOSE_OPTION},
  {"bignum", no_argument, NULL, USE_BIGNUM},
  {"no-bignum", no_argument, NULL, NO_USE_BIGNUM},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
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
      fputs (_("\
      --bignum     always use arbitrary-precision arithmetic\n\
      --no-bignum  always use single-precision arithmetic\n"),
	       stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
Print the prime factors of all specified integer NUMBERs.  If no arguments\n\
are specified on the command line, they are read from standard input.\n\
"), stdout);
      emit_bug_reporting_address ();
    }
  exit (status);
}

static bool
do_stdin (void)
{
  bool ok = true;
  token_buffer tokenbuffer;

  init_tokenbuffer (&tokenbuffer);

  for (;;)
    {
      size_t token_length = readtoken (stdin, DELIM, sizeof (DELIM) - 1,
				       &tokenbuffer);
      if (token_length == (size_t) -1)
	break;
      ok &= print_factors (tokenbuffer.buffer);
    }
  free (tokenbuffer.buffer);

  return ok;
}

int
main (int argc, char **argv)
{
  bool ok;
  int c;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((c = getopt_long (argc, argv, "", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case VERBOSE_OPTION:
	  verbose = true;
	  break;

	case USE_BIGNUM:
	  algorithm = ALGO_GMP;
	  break;

	case NO_USE_BIGNUM:
	  algorithm = ALGO_SINGLE;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (argc <= optind)
    ok = do_stdin ();
  else
    {
      int i;
      ok = true;
      for (i = optind; i < argc; i++)
	if (! print_factors (argv[i]))
	  ok = false;
    }
#if HAVE_GMP
  free (factor);
#endif
  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
