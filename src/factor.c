/* factor -- print prime factors of n.
   Copyright (C) 1986-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Originally written by Paul Rubin <phr@ocf.berkeley.edu>.
   Adapted for GNU, fixed to factor UINT_MAX by Jim Meyering.
   Arbitrary-precision code adapted by James Youngman from Torbjörn
   Granlund's factorize.c, from GNU MP version 4.2.2.
   In 2012, the core was rewritten by Torbjörn Granlund and Niels Möller.
   Contains code from GNU MP.  */

/* Efficiently factor numbers that fit in one or two words (word = wide_uint),
   or, with GMP, numbers of any size.

  Code organization:

    There are several variants of many functions, for handling one word, two
    words, and GMP's mpz_t type.  If the one-word variant is called foo, the
    two-word variant will be foo2, and the one for mpz_t will be mp_foo.  In
    some cases, the plain function variants will handle both one-word and
    two-word numbers, evidenced by function arguments.

    The factoring code for two words will fall into the code for one word when
    progress allows that.

  Algorithm:

    (1) Perform trial division using a small primes table, but without hardware
        division since the primes table store inverses modulo the word base.
        (The GMP variant of this code doesn't make use of the precomputed
        inverses, but instead relies on GMP for fast divisibility testing.)
    (2) Check the nature of any non-factored part using Miller-Rabin for
        detecting composites, and Lucas for detecting primes.
    (3) Factor any remaining composite part using Pollard-Brent rho.
        Status of found factors are checked again using Miller-Rabin and Lucas.

    We prefer using Hensel norm in the divisions, not the more familiar
    Euclidean norm, since the former leads to much faster code.  In the
    Pollard-Brent rho code and the prime testing code, we use Montgomery's
    trick of multiplying all n-residues by the word base, allowing cheap Hensel
    reductions mod n.

    The GMP code uses an algorithm that can be considerably slower;
    for example, on a circa-2017 Intel Xeon Silver 4116, factoring
    2^{127}-3 takes about 50 ms with the two-word algorithm but would
    take about 750 ms with the GMP code.

  Improvements:

    * Use modular inverses also for exact division in the Lucas code, and
      elsewhere.  A problem is to locate the inverses not from an index, but
      from a prime.  We might instead compute the inverse on-the-fly.

    * Tune trial division table size (not forgetting that this is a standalone
      program where the table will be read from secondary storage for
      each invocation).

    * Implement less naive powm, using k-ary exponentiation for k = 3 or
      perhaps k = 4.

    * Try to speed trial division code for single wide_uint numbers, i.e., the
      code using DIVBLOCK.  It currently runs at 2 cycles per prime (Intel SBR,
      IBR), 3 cycles per prime (AMD Stars) and 5 cycles per prime (AMD BD) when
      using gcc 4.6 and 4.7.  Some software pipelining should help; 1, 2, and 4
      respectively cycles ought to be possible.

    * The redcify function could be vastly improved by using (plain Euclidean)
      pre-inversion (such as GMP's invert_limb) and udiv_qrnnd_preinv (from
      GMP's gmp-impl.h).  The redcify2 function could be vastly improved using
      similar methods.  These functions currently dominate run time when using
      the -w option.
*/

/* Whether to recursively factor to prove primality,
   or run faster probabilistic tests.  */
#ifndef PROVE_PRIMALITY
# define PROVE_PRIMALITY 1
#endif


#include <config.h>
#include <getopt.h>
#include <stdbit.h>
#include <stdio.h>
#include <gmp.h>

#include "system.h"
#include "assure.h"
#include "c-ctype.h"
#include "full-write.h"
#include "quote.h"
#include "readtokens.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "factor"

#define AUTHORS \
  proper_name ("Paul Rubin"),                                           \
  proper_name_lite ("Torbjorn Granlund", "Torbj\303\266rn Granlund"),   \
  proper_name_lite ("Niels Moller", "Niels M\303\266ller")

/* Token delimiters when reading from a file.  */
#define DELIM "\n\t "

/* __int128 is experimental; to use it, compile with -DUSE_INT128.  */
#ifndef USE_INT128
# define USE_INT128 false
#endif

/* Typedefs and macros related to an unsigned type that is no narrower
   than 32 bits and no narrower than unsigned int.  For efficiency,
   use the widest hardware-supported type.  */
#if USE_INT128
typedef unsigned __int128 wide_uint;
typedef __int128 wide_int;
# define W_TYPE_SIZE 128
#else
typedef uintmax_t wide_uint;
typedef intmax_t wide_int;
# define W_TYPE_SIZE UINTMAX_WIDTH
#endif
#define WIDE_UINT_MAX ((wide_uint) -1)

#ifndef USE_LONGLONG_H
/* With the way we use longlong.h, it's only safe to use
   when UWtype = UHWtype, as there were various cases
   (as can be seen in the history for longlong.h) where
   for example, _LP64 was required to enable W_TYPE_SIZE==64 code,
   to avoid compile time or run time issues.  */
# define USE_LONGLONG_H (W_TYPE_SIZE == ULONG_WIDTH)
#endif

#if USE_LONGLONG_H

/* Make definitions for longlong.h to make it do what it can do for us */

# define UWtype  wide_uint
# define UHWtype unsigned long int
# undef UDWtype
# if HAVE_ATTRIBUTE_MODE
typedef unsigned int UQItype    __attribute__ ((mode (QI)));
typedef          int SItype     __attribute__ ((mode (SI)));
typedef unsigned int USItype    __attribute__ ((mode (SI)));
typedef          int DItype     __attribute__ ((mode (DI)));
typedef unsigned int UDItype    __attribute__ ((mode (DI)));
# else
typedef unsigned char UQItype;
typedef          long SItype;
typedef unsigned long int USItype;
typedef long long int DItype;
typedef unsigned long long int UDItype;
# endif
# define LONGLONG_STANDALONE     /* Don't require GMP's longlong.h mdep files */

/* FIXME make longlong.h really standalone, so that ASSERT, __GMP_DECLSPEC
   and __GMP_GNUC_PREREQ need not be defined here.  */
# define ASSERT(x)
# define __GMP_DECLSPEC
# ifndef __GMP_GNUC_PREREQ
#  if defined __GNUC__ && defined __GNUC_MINOR__
#   define __GMP_GNUC_PREREQ(a, b) ((a) < __GNUC__ + ((b) <= __GNUC_MINOR__))
#  else
#   define __GMP_GNUC_PREREQ(a, b) 0
#  endif
# endif

/* longlong.h uses these macros only in certain system compiler combinations.
   Ensure usage to pacify -Wunused-macros.  */
# if (defined ASSERT || defined UHWtype \
      || defined __GMP_DECLSPEC || defined __GMP_GNUC_PREREQ)
# endif

# if _ARCH_PPC
#  define HAVE_HOST_CPU_FAMILY_powerpc 1
# endif
# include "longlong.h"

#else /* not USE_LONGLONG_H */

# define __ll_B ((wide_uint) 1 << (W_TYPE_SIZE / 2))
# define __ll_lowpart(t)  ((wide_uint) (t) & (__ll_B - 1))
# define __ll_highpart(t) ((wide_uint) (t) >> (W_TYPE_SIZE / 2))

#endif

/* 2*3*5*7*11...*101 is 128 bits, and has 26 prime factors */
#define MAX_NFACTS 26

enum
{
  DEV_DEBUG_OPTION = CHAR_MAX + 1
};

static struct option const long_options[] =
{
  {"exponents", no_argument, nullptr, 'h'},
  {"-debug", no_argument, nullptr, DEV_DEBUG_OPTION},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
};

/* If true, use p^e output format.  */
static bool print_exponents;

/* This represents an unsigned integer twice as wide as wide_uint.  */
typedef struct { wide_uint uu[2]; } uuint;

/* Accessors and constructors for the type.  Programs should not
   access the type's internals directly, in case some future version
   replaces the type with unsigned __int256 or whatever.  */
static wide_uint lo (uuint u) { return u.uu[0]; }
static wide_uint hi (uuint u) { return u.uu[1]; }
static void hiset (uuint *u, wide_uint hi) { u->uu[1] = hi; }
static bool hi_is_set (uuint const *pu) { return pu->uu[1] != 0; }
static void
uuset (wide_uint *phi, wide_uint *plo, uuint uu)
{
  *phi = hi (uu);
  *plo = lo (uu);
}
static uuint
make_uuint (wide_uint hi, wide_uint lo)
{
  return (uuint) {{lo, hi}};
}

/* BIG_POWER_OF_10 is a positive power of 10 that does not exceed WIDE_UINT_MAX.
   The larger it is, the more efficient the code will likely be.
   LOG_BIG_POWER_OF_10 = log (BIG_POWER_OF_10).  */
#if W_TYPE_SIZE < 64
# error "platform does not support 64-bit integers"
#elif W_TYPE_SIZE < 128
/* A mainstream platforms, with at-least-64-bit uintmax_t.  */
static wide_uint const BIG_POWER_OF_10 = 10000000000000000000llu;
enum { LOG_BIG_POWER_OF_10 = 19 };
#else
/* If built with -DUSE_INT128, a platform that supports __int128; otherwise,
   a so-far-only-theoretical platform with at-least-128-bit uintmax_t.
   This is for performance; the 64-bit mainstream code will still work.  */
static wide_uint const BIG_POWER_OF_10 =
  (wide_uint) 10000000000000000000llu * 10000000000000000000llu;
enum { LOG_BIG_POWER_OF_10 = 38 };
#endif

struct factors
{
  uuint plarge; /* Can have a single large factor */
  wide_uint     p[MAX_NFACTS];
  unsigned char e[MAX_NFACTS];
  unsigned char nfactors;
};

struct mp_factors
{
  mpz_t             *p;
  unsigned long int *e;
  idx_t nfactors;
  idx_t nalloc;
};

static void factor (wide_uint, wide_uint, struct factors *);

#ifndef umul_ppmm
# define umul_ppmm(w1, w0, u, v)                                        \
  do {                                                                  \
    wide_uint __x0, __x1, __x2, __x3;                                   \
    unsigned long int __ul, __vl, __uh, __vh;                           \
    wide_uint __u = (u), __v = (v);                                     \
                                                                        \
    __ul = __ll_lowpart (__u);                                          \
    __uh = __ll_highpart (__u);                                         \
    __vl = __ll_lowpart (__v);                                          \
    __vh = __ll_highpart (__v);                                         \
                                                                        \
    __x0 = (wide_uint) __ul * __vl;                                     \
    __x1 = (wide_uint) __ul * __vh;                                     \
    __x2 = (wide_uint) __uh * __vl;                                     \
    __x3 = (wide_uint) __uh * __vh;                                     \
                                                                        \
    __x1 += __ll_highpart (__x0);/* This can't give carry.  */		\
    if (ckd_add (&__x1, __x1, __x2))	/* Did this give a carry?  */	\
      __x3 += __ll_B;		/* Yes, add it in the proper pos.  */	\
                                                                        \
    (w1) = __x3 + __ll_highpart (__x1);                                 \
    (w0) = (__x1 << W_TYPE_SIZE / 2) + __ll_lowpart (__x0);             \
  } while (0)
#endif

#if !defined udiv_qrnnd || defined UDIV_NEEDS_NORMALIZATION
/* Define our own, not needing normalization.  This function is
   currently not performance critical, so keep it simple.  Similar to
   the mod macro below.  */
# undef udiv_qrnnd
# define udiv_qrnnd(q, r, n1, n0, d)                                    \
  do {                                                                  \
    wide_uint __d1, __d0, __q, __r1, __r0;                              \
                                                                        \
    __d1 = (d); __d0 = 0;                                               \
    __r1 = (n1); __r0 = (n0);                                           \
    affirm (__r1 < __d1);                                               \
    __q = 0;                                                            \
    for (int __i = W_TYPE_SIZE; __i > 0; __i--)                         \
      {                                                                 \
        rsh2 (__d1, __d0, __d1, __d0, 1);                               \
        __q <<= 1;                                                      \
        if (ge2 (__r1, __r0, __d1, __d0))                               \
          {                                                             \
            __q++;                                                      \
            sub_ddmmss (__r1, __r0, __r1, __r0, __d1, __d0);            \
          }                                                             \
      }                                                                 \
    (r) = __r0;                                                         \
    (q) = __q;                                                          \
  } while (0)
#endif

#if !defined add_ssaaaa
# define add_ssaaaa(sh, sl, ah, al, bh, bl)                             \
  do {                                                                  \
    wide_uint _add_x;							\
    (sh) = (ah) + (bh) + ckd_add (&_add_x, al, bl);			\
    (sl) = _add_x;                                                      \
  } while (0)
#endif

/* Set (rh,rl) = (ah,al) >> cnt, where 0 < cnt < W_TYPE_SIZE.  */
#define rsh2(rh, rl, ah, al, cnt)                                       \
  do {                                                                  \
    (rl) = ((ah) << (W_TYPE_SIZE - (cnt))) | ((al) >> (cnt));           \
    (rh) = (ah) >> (cnt);                                               \
  } while (0)

/* Set (rh,rl) = (ah,al) << cnt, where 0 < cnt < W_TYPE_SIZE.  */
#define lsh2(rh, rl, ah, al, cnt)                                       \
  do {                                                                  \
    (rh) = ((ah) << cnt) | ((al) >> (W_TYPE_SIZE - (cnt)));             \
    (rl) = (al) << (cnt);                                               \
  } while (0)

#define ge2(ah, al, bh, bl)                                             \
  ((ah) > (bh) || ((ah) == (bh) && (al) >= (bl)))

#define gt2(ah, al, bh, bl)                                             \
  ((ah) > (bh) || ((ah) == (bh) && (al) > (bl)))

#ifndef sub_ddmmss
# define sub_ddmmss(rh, rl, ah, al, bh, bl)                             \
   ((rh) = (ah) - (bh) - ckd_sub (&(rl), al, bl))
#endif

/* Requires that a < n and b <= n */
#define submod(r,a,b,n)                                                 \
  do {                                                                  \
    wide_uint _s, _t = -ckd_sub (&_s, a, b);				\
    (r) = ((n) & _t) + _s;						\
  } while (0)

#define addmod(r,a,b,n)                                                 \
  submod ((r), (a), ((n) - (b)), (n))

/* Modular two-word addition and subtraction.  For performance reasons, the
   most significant bit of n1 must be clear.  The destination variables must be
   distinct from the mod operand.  */
#define addmod2(r1, r0, a1, a0, b1, b0, n1, n0)                         \
  do {                                                                  \
    add_ssaaaa ((r1), (r0), (a1), (a0), (b1), (b0));                    \
    if (ge2 ((r1), (r0), (n1), (n0)))                                   \
      sub_ddmmss ((r1), (r0), (r1), (r0), (n1), (n0));                  \
  } while (0)
#define submod2(r1, r0, a1, a0, b1, b0, n1, n0)                         \
  do {                                                                  \
    sub_ddmmss ((r1), (r0), (a1), (a0), (b1), (b0));                    \
    if ((wide_int) (r1) < 0)                                            \
      add_ssaaaa ((r1), (r0), (r1), (r0), (n1), (n0));                  \
  } while (0)

#define HIGHBIT_TO_MASK(x)                                              \
  (((wide_int)-1 >> 1) < 0                                              \
   ? (wide_uint)((wide_int)(x) >> (W_TYPE_SIZE - 1))                    \
   : ((x) & ((wide_uint) 1 << (W_TYPE_SIZE - 1))                        \
      ? WIDE_UINT_MAX : (wide_uint) 0))

/* Return r = a mod d, where a = <a1,a0>, d = <d1,d0>.
   Requires that d1 != 0.  */
ATTRIBUTE_PURE static uuint
mod2 (wide_uint a1, wide_uint a0, wide_uint d1, wide_uint d0)
{
  affirm (d1 != 0);

  if (a1)
    {
      int cntd = stdc_leading_zeros (d1);
      int cnta = stdc_leading_zeros (a1);
      int cnt = cntd - cnta;
      if (0 < cnt)
        {
          lsh2 (d1, d0, d1, d0, cnt);
          for (int i = 0; i < cnt; i++)
            {
              if (ge2 (a1, a0, d1, d0))
                sub_ddmmss (a1, a0, a1, a0, d1, d0);
              rsh2 (d1, d0, d1, d0, 1);
            }
        }
    }

  return make_uuint (a1, a0);
}

ATTRIBUTE_CONST
static wide_uint
gcd_odd (wide_uint a, wide_uint b)
{
  if ((b & 1) == 0)
    {
      wide_uint t = b;
      b = a;
      a = t;
    }
  if (a == 0)
    return b;

  /* Take out least significant one bit, to make room for sign */
  b >>= 1;

  for (;;)
    {
      wide_uint t;
      wide_uint bgta;

      assume (a);
      a >>= stdc_trailing_zeros (a);
      a >>= 1;

      t = a - b;
      if (t == 0)
        return (a << 1) + 1;

      bgta = HIGHBIT_TO_MASK (t);

      /* b <-- min (a, b) */
      b += (bgta & t);

      /* a <-- |a - b| */
      a = (t ^ bgta) - bgta;
    }
}

ATTRIBUTE_PURE static uuint
gcd2_odd (wide_uint a1, wide_uint a0, wide_uint b1, wide_uint b0)
{
  affirm (b0 & 1);

  if ((a0 | a1) == 0)
    return make_uuint (b1, b0);
  if (!a0)
    a0 = a1, a1 = 0;
  assume (a0);
  int ctz = stdc_trailing_zeros (a0);
  if (ctz)
    rsh2 (a1, a0, a1, a0, ctz);

  for (;;)
    {
      if ((b1 | a1) == 0)
        return make_uuint (0, gcd_odd (b0, a0));

      if (gt2 (a1, a0, b1, b0))
        {
          sub_ddmmss (a1, a0, a1, a0, b1, b0);
          if (!a0)
            a0 = a1, a1 = 0;
          assume (a0);
          ctz = stdc_trailing_zeros (a0);
          if (ctz)
            rsh2 (a1, a0, a1, a0, ctz);
        }
      else if (gt2 (b1, b0, a1, a0))
        {
          sub_ddmmss (b1, b0, b1, b0, a1, a0);
          if (!b0)
            b0 = b1, b1 = 0;
          assume (b0);
          ctz = stdc_trailing_zeros (b0);
          if (ctz)
            rsh2 (b1, b0, b1, b0, ctz);
        }
      else
        break;
    }

  return make_uuint (a1, a0);
}

static void
factor_insert_multiplicity (struct factors *factors,
                            wide_uint prime, int m)
{
  int nfactors = factors->nfactors;
  wide_uint *p = factors->p;
  unsigned char *e = factors->e;

  /* Locate position for insert new or increment e.  */
  int i;
  for (i = nfactors - 1; i >= 0; i--)
    {
      if (p[i] <= prime)
        break;
    }

  if (i < 0 || p[i] != prime)
    {
      for (int j = nfactors - 1; j > i; j--)
        {
          p[j + 1] = p[j];
          e[j + 1] = e[j];
        }
      p[i + 1] = prime;
      e[i + 1] = m;
      factors->nfactors = nfactors + 1;
    }
  else
    {
      e[i] += m;
    }
}

#define factor_insert(f, p) factor_insert_multiplicity (f, p, 1)

static void
factor_insert_large (struct factors *factors,
                     wide_uint p1, wide_uint p0)
{
  if (p1 > 0)
    {
      affirm (!hi_is_set (&factors->plarge));
      factors->plarge = make_uuint (p1, p0);
    }
  else
    factor_insert (factors, p0);
}

#ifndef mpz_inits

# include <stdarg.h>

# define mpz_inits(...) mpz_va_init (mpz_init, __VA_ARGS__)
# define mpz_clears(...) mpz_va_init (mpz_clear, __VA_ARGS__)

static void
mpz_va_init (void (*mpz_single_init)(mpz_t), ...)
{
  va_list ap;

  va_start (ap, mpz_single_init);

  mpz_t *mpz;
  while ((mpz = va_arg (ap, mpz_t *)))
    mpz_single_init (*mpz);

  va_end (ap);
}
#endif

static void mp_factor (mpz_t, struct mp_factors *);

static void
mp_factor_init (struct mp_factors *factors)
{
  factors->p = nullptr;
  factors->e = nullptr;
  factors->nfactors = 0;
  factors->nalloc = 0;
}

static void
mp_factor_clear (struct mp_factors *factors)
{
  for (idx_t i = 0; i < factors->nfactors; i++)
    mpz_clear (factors->p[i]);

  free (factors->p);
  free (factors->e);
}

static void
mp_factor_insert (struct mp_factors *factors, mpz_t prime)
{
  idx_t nfactors = factors->nfactors;
  mpz_t *p = factors->p;
  unsigned long int *e = factors->e;
  ptrdiff_t i;

  /* Locate position for insert new or increment e.  */
  for (i = nfactors - 1; i >= 0; i--)
    {
      if (mpz_cmp (p[i], prime) <= 0)
        break;
    }

  if (i < 0 || mpz_cmp (p[i], prime) != 0)
    {
      if (factors->nfactors == factors->nalloc)
        {
          p = xpalloc (p, &factors->nalloc, 1, -1, sizeof *p);
          e = xireallocarray (e, factors->nalloc, sizeof *e);
        }

      mpz_init (p[nfactors]);
      for (ptrdiff_t j = nfactors - 1; j > i; j--)
        {
          mpz_set (p[j + 1], p[j]);
          e[j + 1] = e[j];
        }
      mpz_set (p[i + 1], prime);
      e[i + 1] = 1;

      factors->p = p;
      factors->e = e;
      factors->nfactors = nfactors + 1;
    }
  else
    {
      e[i] += 1;
    }
}

static void
mp_factor_insert_ui (struct mp_factors *factors, unsigned long int prime)
{
  mpz_t pz;

  mpz_init_set_ui (pz, prime);
  mp_factor_insert (factors, pz);
  mpz_clear (pz);
}


#define P(a,b,c,d) a,
static const unsigned char primes_diff[] = {
#include "primes.h"
0,0,0,0,0,0,0                           /* 7 sentinels for 8-way loop */
};
#undef P

#define PRIMES_PTAB_ENTRIES (ARRAY_CARDINALITY (primes_diff) - 8 + 1)

#define P(a,b,c,d) b,
static const unsigned char primes_diff8[] = {
#include "primes.h"
0,0,0,0,0,0,0                           /* 7 sentinels for 8-way loop */
};
#undef P

struct primes_dtab
{
  wide_uint binv, lim;
};

#define P(a,b,c,d) {c,d},
static const struct primes_dtab primes_dtab[] = {
#include "primes.h"
{1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0} /* 7 sentinels for 8-way loop */
};
#undef P

/* Verify that wide_uint is not wider than
   the integers used to generate primes.h.  */
static_assert (W_TYPE_SIZE <= WIDE_UINT_BITS);

/* debugging for developers.  Enables devmsg().
   This flag is used only in the GMP code.  */
static bool dev_debug = false;

/* Prove primality or run probabilistic tests.  */
static bool flag_prove_primality = PROVE_PRIMALITY;

/* Number of Miller-Rabin tests to run when not proving primality.  */
#define MR_REPS 25

static void
factor_insert_refind (struct factors *factors, wide_uint p, int i, int off)
{
  for (int j = 0; j < off; j++)
    p += primes_diff[i + j];
  factor_insert (factors, p);
}

/* Trial division with odd primes uses the following trick.

   Let p be an odd prime, and B = 2^{W_TYPE_SIZE}.  For simplicity,
   consider the case t < B (this is the second loop below).

   From our tables we get

     binv = p^{-1} (mod B)
     lim = floor ((B-1) / p).

   First assume that t is a multiple of p, t = q * p.  Then 0 <= q <= lim
   (and all quotients in this range occur for some t).

   Then t = q * p is true also (mod B), and p is invertible we get

     q = t * binv (mod B).

   Next, assume that t is *not* divisible by p.  Since multiplication
   by binv (mod B) is a one-to-one mapping,

     t * binv (mod B) > lim,

   because all the smaller values are already taken.

   This can be summed up by saying that the function

     q(t) = binv * t (mod B)

   is a permutation of the range 0 <= t < B, with the curious property
   that it maps the multiples of p onto the range 0 <= q <= lim, in
   order, and the non-multiples of p onto the range lim < q < B.
 */

static uuint
factor_using_division (wide_uint t1, wide_uint t0,
                       struct factors *factors)
{
  if (t0 % 2 == 0)
    {
      int cnt;

      if (t0 == 0)
        {
          assume (t1);
          cnt = stdc_trailing_zeros (t1);
          t0 = t1 >> cnt;
          t1 = 0;
          cnt += W_TYPE_SIZE;
        }
      else
        {
          cnt = stdc_trailing_zeros (t0);
          rsh2 (t1, t0, t1, t0, cnt);
        }

      factor_insert_multiplicity (factors, 2, cnt);
    }

  wide_uint p = 3;
  idx_t i;
  for (i = 0; t1 > 0 && i < PRIMES_PTAB_ENTRIES; i++)
    {
      for (;;)
        {
          wide_uint q1, q0, hi;
          MAYBE_UNUSED wide_uint lo;

          q0 = t0 * primes_dtab[i].binv;
          umul_ppmm (hi, lo, q0, p);
          if (hi > t1)
            break;
          hi = t1 - hi;
          q1 = hi * primes_dtab[i].binv;
          if (LIKELY (q1 > primes_dtab[i].lim))
            break;
          t1 = q1; t0 = q0;
          factor_insert (factors, p);
        }
      p += primes_diff[i + 1];
    }

#define DIVBLOCK(I)                                                     \
  do {                                                                  \
    for (;;)                                                            \
      {                                                                 \
        q = t0 * pd[I].binv;                                            \
        if (LIKELY (q > pd[I].lim))                                     \
          break;                                                        \
        t0 = q;                                                         \
        factor_insert_refind (factors, p, i + 1, I);                    \
      }                                                                 \
  } while (0)

  for (; i < PRIMES_PTAB_ENTRIES; i += 8)
    {
      wide_uint q;
      const struct primes_dtab *pd = &primes_dtab[i];
      DIVBLOCK (0);
      DIVBLOCK (1);
      DIVBLOCK (2);
      DIVBLOCK (3);
      DIVBLOCK (4);
      DIVBLOCK (5);
      DIVBLOCK (6);
      DIVBLOCK (7);

      p += primes_diff8[i];
      if (p * p > t0)
        break;
    }

  return make_uuint (t1, t0);
}

static void
mp_factor_using_division (mpz_t t, struct mp_factors *factors)
{
  mpz_t q;
  mp_bitcnt_t p;

  devmsg ("[trial division] ");

  mpz_init (q);

  p = mpz_scan1 (t, 0);
  mpz_fdiv_q_2exp (t, t, p);
  while (p)
    {
      mp_factor_insert_ui (factors, 2);
      --p;
    }

  unsigned long int d = 3;
  for (idx_t i = 1; i <= PRIMES_PTAB_ENTRIES;)
    {
      if (! mpz_divisible_ui_p (t, d))
        {
          d += primes_diff[i++];
          if (mpz_cmp_ui (t, d * d) < 0)
            break;
        }
      else
        {
          mpz_tdiv_q_ui (t, t, d);
          mp_factor_insert_ui (factors, d);
        }
    }

  mpz_clear (q);
}

/* Entry i contains (2i+1)^(-1) mod 2^8.  */
static const unsigned char  binvert_table[128] =
{
  0x01, 0xAB, 0xCD, 0xB7, 0x39, 0xA3, 0xC5, 0xEF,
  0xF1, 0x1B, 0x3D, 0xA7, 0x29, 0x13, 0x35, 0xDF,
  0xE1, 0x8B, 0xAD, 0x97, 0x19, 0x83, 0xA5, 0xCF,
  0xD1, 0xFB, 0x1D, 0x87, 0x09, 0xF3, 0x15, 0xBF,
  0xC1, 0x6B, 0x8D, 0x77, 0xF9, 0x63, 0x85, 0xAF,
  0xB1, 0xDB, 0xFD, 0x67, 0xE9, 0xD3, 0xF5, 0x9F,
  0xA1, 0x4B, 0x6D, 0x57, 0xD9, 0x43, 0x65, 0x8F,
  0x91, 0xBB, 0xDD, 0x47, 0xC9, 0xB3, 0xD5, 0x7F,
  0x81, 0x2B, 0x4D, 0x37, 0xB9, 0x23, 0x45, 0x6F,
  0x71, 0x9B, 0xBD, 0x27, 0xA9, 0x93, 0xB5, 0x5F,
  0x61, 0x0B, 0x2D, 0x17, 0x99, 0x03, 0x25, 0x4F,
  0x51, 0x7B, 0x9D, 0x07, 0x89, 0x73, 0x95, 0x3F,
  0x41, 0xEB, 0x0D, 0xF7, 0x79, 0xE3, 0x05, 0x2F,
  0x31, 0x5B, 0x7D, 0xE7, 0x69, 0x53, 0x75, 0x1F,
  0x21, 0xCB, 0xED, 0xD7, 0x59, 0xC3, 0xE5, 0x0F,
  0x11, 0x3B, 0x5D, 0xC7, 0x49, 0x33, 0x55, 0xFF
};

/* Compute n^(-1) mod B, using a Newton iteration.  */
#define binv(inv,n)                                                     \
  do {                                                                  \
    wide_uint  __n = (n);                                               \
    wide_uint  __inv;                                                   \
                                                                        \
    __inv = binvert_table[(__n / 2) & 0x7F]; /*  8 */                   \
    if (W_TYPE_SIZE > 8)   __inv = 2 * __inv - __inv * __inv * __n;     \
    if (W_TYPE_SIZE > 16)  __inv = 2 * __inv - __inv * __inv * __n;     \
    if (W_TYPE_SIZE > 32)  __inv = 2 * __inv - __inv * __inv * __n;     \
                                                                        \
    if (W_TYPE_SIZE > 64)                                               \
      {                                                                 \
        int  __invbits = 64;                                            \
        do {                                                            \
          __inv = 2 * __inv - __inv * __inv * __n;                      \
          __invbits *= 2;                                               \
        } while (__invbits < W_TYPE_SIZE);                              \
      }                                                                 \
                                                                        \
    (inv) = __inv;                                                      \
  } while (0)

/* q = u / d, assuming d|u.  */
#define divexact_21(q1, q0, u1, u0, d)                                  \
  do {                                                                  \
    wide_uint _di, _q0;                                                 \
    binv (_di, (d));                                                    \
    _q0 = (u0) * _di;                                                   \
    if ((u1) >= (d))                                                    \
      {                                                                 \
        wide_uint _p1;                                                  \
        MAYBE_UNUSED wide_int _p0;                                      \
        umul_ppmm (_p1, _p0, _q0, d);                                   \
        (q1) = ((u1) - _p1) * _di;                                      \
        (q0) = _q0;                                                     \
      }                                                                 \
    else                                                                \
      {                                                                 \
        (q0) = _q0;                                                     \
        (q1) = 0;                                                       \
      }                                                                 \
  } while (0)

/* x B (mod n).  */
#define redcify(r_prim, r, n)                                           \
  do {                                                                  \
    MAYBE_UNUSED wide_uint _redcify_q;					\
    udiv_qrnnd (_redcify_q, r_prim, r, 0, n);                           \
  } while (0)

/* x B^2 (mod n).  Requires x > 0, n1 < B/2.  */
#define redcify2(r1, r0, x, n1, n0)                                     \
  do {                                                                  \
    wide_uint _r1, _r0, _i;                                             \
    if ((x) < (n1))                                                     \
      {                                                                 \
        _r1 = (x); _r0 = 0;                                             \
        _i = W_TYPE_SIZE;                                               \
      }                                                                 \
    else                                                                \
      {                                                                 \
        _r1 = 0; _r0 = (x);                                             \
        _i = 2 * W_TYPE_SIZE;                                           \
      }                                                                 \
    while (_i-- > 0)                                                    \
      {                                                                 \
        lsh2 (_r1, _r0, _r1, _r0, 1);                                   \
        if (ge2 (_r1, _r0, (n1), (n0)))                                 \
          sub_ddmmss (_r1, _r0, _r1, _r0, (n1), (n0));                  \
      }                                                                 \
    (r1) = _r1;                                                         \
    (r0) = _r0;                                                         \
  } while (0)

/* Modular two-word multiplication, r = a * b mod m, with mi = m^(-1) mod B.
   Both a and b must be in redc form, the result will be in redc form too.  */
static inline wide_uint
mulredc (wide_uint a, wide_uint b, wide_uint m, wide_uint mi)
{
  wide_uint rh, rl, q, th, xh;
  MAYBE_UNUSED wide_uint tl;

  umul_ppmm (rh, rl, a, b);
  q = rl * mi;
  umul_ppmm (th, tl, q, m);
  if (ckd_sub (&xh, rh, th))
    xh += m;

  return xh;
}

/* Modular two-word multiplication, r = a * b mod m, with mi = m^(-1) mod B.
   Both a and b must be in redc form, the result will be in redc form too.
   For performance reasons, the most significant bit of m must be clear.  */
static wide_uint
mulredc2 (wide_uint *r1p,
          wide_uint a1, wide_uint a0, wide_uint b1, wide_uint b0,
          wide_uint m1, wide_uint m0, wide_uint mi)
{
  wide_uint r1, r0, q, p1, t1, t0, s1, s0;
  MAYBE_UNUSED wide_uint p0;
  mi = -mi;
  affirm ((m1 >> (W_TYPE_SIZE - 1)) == 0);

  /* First compute a0 * <b1, b0> B^{-1}
        +-----+
        |a0 b0|
     +--+--+--+
     |a0 b1|
     +--+--+--+
        |q0 m0|
     +--+--+--+
     |q0 m1|
    -+--+--+--+
     |r1|r0| 0|
     +--+--+--+
  */
  umul_ppmm (t1, t0, a0, b0);
  umul_ppmm (r1, r0, a0, b1);
  q = mi * t0;
  umul_ppmm (p1, p0, q, m0);
  umul_ppmm (s1, s0, q, m1);
  r0 += (t0 != 0); /* Carry */
  add_ssaaaa (r1, r0, r1, r0, 0, p1);
  add_ssaaaa (r1, r0, r1, r0, 0, t1);
  add_ssaaaa (r1, r0, r1, r0, s1, s0);

  /* Next, (a1 * <b1, b0> + <r1, r0> B^{-1}
        +-----+
        |a1 b0|
        +--+--+
        |r1|r0|
     +--+--+--+
     |a1 b1|
     +--+--+--+
        |q1 m0|
     +--+--+--+
     |q1 m1|
    -+--+--+--+
     |r1|r0| 0|
     +--+--+--+
  */
  umul_ppmm (t1, t0, a1, b0);
  umul_ppmm (s1, s0, a1, b1);
  add_ssaaaa (t1, t0, t1, t0, 0, r0);
  q = mi * t0;
  add_ssaaaa (r1, r0, s1, s0, 0, r1);
  umul_ppmm (p1, p0, q, m0);
  umul_ppmm (s1, s0, q, m1);
  r0 += (t0 != 0); /* Carry */
  add_ssaaaa (r1, r0, r1, r0, 0, p1);
  add_ssaaaa (r1, r0, r1, r0, 0, t1);
  add_ssaaaa (r1, r0, r1, r0, s1, s0);

  if (ge2 (r1, r0, m1, m0))
    sub_ddmmss (r1, r0, r1, r0, m1, m0);

  *r1p = r1;
  return r0;
}

ATTRIBUTE_CONST
static wide_uint
powm (wide_uint b, wide_uint e, wide_uint n, wide_uint ni, wide_uint one)
{
  wide_uint y = one;

  if (e & 1)
    y = b;

  while (e != 0)
    {
      b = mulredc (b, b, n, ni);
      e >>= 1;

      if (e & 1)
        y = mulredc (y, b, n, ni);
    }

  return y;
}

ATTRIBUTE_PURE static uuint
powm2 (const wide_uint *bp, const wide_uint *ep, const wide_uint *np,
       wide_uint ni, const wide_uint *one)
{
  wide_uint r1, r0, b1, b0, n1, n0;
  int i;
  wide_uint e;

  b0 = bp[0];
  b1 = bp[1];
  n0 = np[0];
  n1 = np[1];

  r0 = one[0];
  r1 = one[1];

  for (e = ep[0], i = W_TYPE_SIZE; i > 0; i--, e >>= 1)
    {
      if (e & 1)
        {
          wide_uint r1m1;
          r0 = mulredc2 (&r1m1, r1, r0, b1, b0, n1, n0, ni);
          r1 = r1m1;
        }
      wide_uint r1m;
      b0 = mulredc2 (&r1m, b1, b0, b1, b0, n1, n0, ni);
      b1 = r1m;
    }
  for (e = ep[1]; e > 0; e >>= 1)
    {
      if (e & 1)
        {
          wide_uint r1m1;
          r0 = mulredc2 (&r1m1, r1, r0, b1, b0, n1, n0, ni);
          r1 = r1m1;
        }
      wide_uint r1m;
      b0 = mulredc2 (&r1m, b1, b0, b1, b0, n1, n0, ni);
      b1 = r1m;
    }
  return make_uuint (r1, r0);
}

ATTRIBUTE_CONST
static bool
millerrabin (wide_uint n, wide_uint ni, wide_uint b, wide_uint q,
             int k, wide_uint one)
{
  wide_uint y = powm (b, q, n, ni, one);

  wide_uint nm1 = n - one;      /* -1, but in redc representation.  */

  if (y == one || y == nm1)
    return true;

  for (int i = 1; i < k; i++)
    {
      y = mulredc (y, y, n, ni);

      if (y == nm1)
        return true;
      if (y == one)
        return false;
    }
  return false;
}

ATTRIBUTE_PURE static bool
millerrabin2 (const wide_uint *np, wide_uint ni, const wide_uint *bp,
              const wide_uint *qp, int k, const wide_uint *one)
{
  wide_uint y1, y0, nm1_1, nm1_0, r1m;

  uuset (&y1, &y0, powm2 (bp, qp, np, ni, one));

  if (y0 == one[0] && y1 == one[1])
    return true;

  sub_ddmmss (nm1_1, nm1_0, np[1], np[0], one[1], one[0]);

  if (y0 == nm1_0 && y1 == nm1_1)
    return true;

  for (int i = 1; i < k; i++)
    {
      y0 = mulredc2 (&r1m, y1, y0, y1, y0, np[1], np[0], ni);
      y1 = r1m;

      if (y0 == nm1_0 && y1 == nm1_1)
        return true;
      if (y0 == one[0] && y1 == one[1])
        return false;
    }
  return false;
}

static bool
mp_millerrabin (mpz_srcptr n, mpz_srcptr nm1, mpz_ptr x, mpz_ptr y,
                mpz_srcptr q, mp_bitcnt_t k)
{
  mpz_powm (y, x, q, n);

  if (mpz_cmp_ui (y, 1) == 0 || mpz_cmp (y, nm1) == 0)
    return true;

  for (mp_bitcnt_t i = 1; i < k; i++)
    {
      mpz_powm_ui (y, y, 2, n);
      if (mpz_cmp (y, nm1) == 0)
        return true;
      if (mpz_cmp_ui (y, 1) == 0)
        return false;
    }
  return false;
}

/* Lucas' prime test.  The number of iterations vary greatly, up to a few dozen
   have been observed.  The average seem to be about 2.  */
static bool ATTRIBUTE_PURE
prime_p (wide_uint n)
{
  bool is_prime;
  wide_uint a_prim, one, ni;
  struct factors factors;

  if (n <= 1)
    return false;

  wide_uint cast_out_limit
    = (wide_uint) FIRST_OMITTED_PRIME * FIRST_OMITTED_PRIME;

#ifndef EXHIBIT_INT128_BUG
  /* FIXME: Do the small-prime performance improvement only if
     wide_uint is exactly 64 bits wide.  We don't know why the code
     misbehaves when wide_uint is wider; e.g., when compiled with
     'gcc -DUSE_INT128 -DEXHIBIT_INT128_BUG', 'factor' mishandles
     340282366920938463463374607431768211355.  */
  if (W_TYPE_SIZE != 64)
    cast_out_limit = 2;
#endif

  /* We have already cast out small primes.  */
  if (n < cast_out_limit)
    return true;

  /* Precomputation for Miller-Rabin.  */
  int k = stdc_trailing_zeros (n - 1);
  wide_uint q = (n - 1) >> k;

  wide_uint a = 2;
  binv (ni, n);                 /* ni <- 1/n mod B */
  redcify (one, 1, n);
  addmod (a_prim, one, one, n); /* i.e., redcify a = 2 */

  /* Perform a Miller-Rabin test, finds most composites quickly.  */
  if (!millerrabin (n, ni, a_prim, q, k, one))
    return false;

  if (flag_prove_primality)
    {
      /* Factor n-1 for Lucas.  */
      factor (0, n - 1, &factors);
    }

  /* Loop until Lucas proves our number prime, or Miller-Rabin proves our
     number composite.  */
  for (idx_t r = 0; r < PRIMES_PTAB_ENTRIES; r++)
    {
      if (flag_prove_primality)
        {
          is_prime = true;
          for (int i = 0; i < factors.nfactors && is_prime; i++)
            {
              is_prime
                = powm (a_prim, (n - 1) / factors.p[i], n, ni, one) != one;
            }
        }
      else
        {
          /* After enough Miller-Rabin runs, be content.  */
          is_prime = (r == MR_REPS - 1);
        }

      if (is_prime)
        return true;

      a += primes_diff[r];      /* Establish new base.  */

      /* The following is equivalent to redcify (a_prim, a, n).  It runs faster
         on most processors, since it avoids udiv_qrnnd.  If we go down the
         udiv_qrnnd_preinv path, this code should be replaced.  */
      {
        wide_uint s1, s0;
        umul_ppmm (s1, s0, one, a);
        if (LIKELY (s1 == 0))
          a_prim = s0 % n;
        else
          {
            MAYBE_UNUSED wide_uint dummy;
            udiv_qrnnd (dummy, a_prim, s1, s0, n);
          }
      }

      if (!millerrabin (n, ni, a_prim, q, k, one))
        return false;
    }

  affirm (!"Lucas prime test failure.  This should not happen");
}

static bool ATTRIBUTE_PURE
prime2_p (wide_uint n1, wide_uint n0)
{
  wide_uint q[2], nm1[2];
  wide_uint a_prim[2];
  wide_uint one[2];
  wide_uint na[2];
  wide_uint ni;
  int k;
  struct factors factors;

  if (n1 == 0)
    return prime_p (n0);

  nm1[1] = n1 - (n0 == 0);
  nm1[0] = n0 - 1;
  if (nm1[0] == 0)
    {
      assume (nm1[1]);
      k = stdc_trailing_zeros (nm1[1]);

      q[0] = nm1[1] >> k;
      q[1] = 0;
      k += W_TYPE_SIZE;
    }
  else
    {
      k = stdc_trailing_zeros (nm1[0]);
      rsh2 (q[1], q[0], nm1[1], nm1[0], k);
    }

  wide_uint a = 2;
  binv (ni, n0);
  redcify2 (one[1], one[0], 1, n1, n0);
  addmod2 (a_prim[1], a_prim[0], one[1], one[0], one[1], one[0], n1, n0);

  /* FIXME: Use scalars or pointers in arguments?  Some consistency needed.  */
  na[0] = n0;
  na[1] = n1;

  if (!millerrabin2 (na, ni, a_prim, q, k, one))
    return false;

  if (flag_prove_primality)
    {
      /* Factor n-1 for Lucas.  */
      factor (nm1[1], nm1[0], &factors);
    }

  /* Loop until Lucas proves our number prime, or Miller-Rabin proves our
     number composite.  */
  for (idx_t r = 0; r < PRIMES_PTAB_ENTRIES; r++)
    {
      bool is_prime;
      wide_uint e[2];
      uuint y;

      if (flag_prove_primality)
        {
          is_prime = true;
          if (hi_is_set (&factors.plarge))
            {
              wide_uint pi;
              binv (pi, lo (factors.plarge));
              e[0] = pi * nm1[0];
              e[1] = 0;
              y = powm2 (a_prim, e, na, ni, one);
              is_prime = (lo (y) != one[0] || hi (y) != one[1]);
            }
          for (int i = 0; i < factors.nfactors && is_prime; i++)
            {
              /* FIXME: We always have the factor 2.  Do we really need to
                 handle it here?  We have done the same powering as part
                 of millerrabin.  */
              if (factors.p[i] == 2)
                rsh2 (e[1], e[0], nm1[1], nm1[0], 1);
              else
                divexact_21 (e[1], e[0], nm1[1], nm1[0], factors.p[i]);
              y = powm2 (a_prim, e, na, ni, one);
              is_prime = (lo (y) != one[0] || hi (y) != one[1]);
            }
        }
      else
        {
          /* After enough Miller-Rabin runs, be content.  */
          is_prime = (r == MR_REPS - 1);
        }

      if (is_prime)
        return true;

      a += primes_diff[r];      /* Establish new base.  */
      redcify2 (a_prim[1], a_prim[0], a, n1, n0);

      if (!millerrabin2 (na, ni, a_prim, q, k, one))
        return false;
    }

  affirm (!"Lucas prime test failure.  This should not happen");
}

static bool
mp_prime_p (mpz_t n)
{
  bool is_prime;
  mpz_t q, a, nm1, tmp;
  struct mp_factors factors;

  if (mpz_cmp_ui (n, 1) <= 0)
    return false;

  /* We have already cast out small primes.  */
  if (mpz_cmp_ui (n, (long) FIRST_OMITTED_PRIME * FIRST_OMITTED_PRIME) < 0)
    return true;

  mpz_inits (q, a, nm1, tmp, nullptr);

  /* Precomputation for Miller-Rabin.  */
  mpz_sub_ui (nm1, n, 1);

  /* Find q and k, where q is odd and n = 1 + 2**k * q.  */
  mp_bitcnt_t k = mpz_scan1 (nm1, 0);
  mpz_tdiv_q_2exp (q, nm1, k);

  mpz_set_ui (a, 2);

  /* Perform a Miller-Rabin test, finds most composites quickly.  */
  if (!mp_millerrabin (n, nm1, a, tmp, q, k))
    {
      is_prime = false;
      goto ret2;
    }

  if (flag_prove_primality)
    {
      /* Factor n-1 for Lucas.  */
      mpz_set (tmp, nm1);
      mp_factor (tmp, &factors);
    }

  /* Loop until Lucas proves our number prime, or Miller-Rabin proves our
     number composite.  */
  for (idx_t r = 0; r < PRIMES_PTAB_ENTRIES; r++)
    {
      if (flag_prove_primality)
        {
          is_prime = true;
          for (idx_t i = 0; i < factors.nfactors && is_prime; i++)
            {
              mpz_divexact (tmp, nm1, factors.p[i]);
              mpz_powm (tmp, a, tmp, n);
              is_prime = mpz_cmp_ui (tmp, 1) != 0;
            }
        }
      else
        {
          /* After enough Miller-Rabin runs, be content.  */
          is_prime = (r == MR_REPS - 1);
        }

      if (is_prime)
        goto ret1;

      mpz_add_ui (a, a, primes_diff[r]);        /* Establish new base.  */

      if (!mp_millerrabin (n, nm1, a, tmp, q, k))
        {
          is_prime = false;
          goto ret1;
        }
    }

  affirm (!"Lucas prime test failure.  This should not happen");

 ret1:
  if (flag_prove_primality)
    mp_factor_clear (&factors);
 ret2:
  mpz_clears (q, a, nm1, tmp, nullptr);

  return is_prime;
}

static void
factor_using_pollard_rho (wide_uint n, unsigned long int a,
                          struct factors *factors)
{
  wide_uint x, z, y, P, t, ni, g;

  unsigned long int k = 1;
  unsigned long int l = 1;

  redcify (P, 1, n);
  addmod (x, P, P, n);          /* i.e., redcify(2) */
  y = z = x;

  while (n != 1)
    {
      affirm (a < n);

      binv (ni, n);             /* FIXME: when could we use old 'ni' value?  */

      for (;;)
        {
          do
            {
              x = mulredc (x, x, n, ni);
              addmod (x, x, a, n);

              submod (t, z, x, n);
              P = mulredc (P, t, n, ni);

              if (k % 32 == 1)
                {
                  if (gcd_odd (P, n) != 1)
                    goto factor_found;
                  y = x;
                }
            }
          while (--k != 0);

          z = x;
          k = l;
          l = 2 * l;
          for (unsigned long int i = 0; i < k; i++)
            {
              x = mulredc (x, x, n, ni);
              addmod (x, x, a, n);
            }
          y = x;
        }

    factor_found:
      do
        {
          y = mulredc (y, y, n, ni);
          addmod (y, y, a, n);

          submod (t, z, y, n);
          g = gcd_odd (t, n);
        }
      while (g == 1);

      if (n == g)
        {
          /* Found n itself as factor.  Restart with different params.  */
          factor_using_pollard_rho (n, a + 1, factors);
          return;
        }

      n = n / g;

      if (!prime_p (g))
        factor_using_pollard_rho (g, a + 1, factors);
      else
        factor_insert (factors, g);

      if (prime_p (n))
        {
          factor_insert (factors, n);
          break;
        }

      x = x % n;
      z = z % n;
      y = y % n;
    }
}

static void
factor_using_pollard_rho2 (wide_uint n1, wide_uint n0, unsigned long int a,
                           struct factors *factors)
{
  wide_uint x1, x0, z1, z0, y1, y0, P1, P0, t1, t0, ni, g1, g0, r1m;

  unsigned long int k = 1;
  unsigned long int l = 1;

  redcify2 (P1, P0, 1, n1, n0);
  addmod2 (x1, x0, P1, P0, P1, P0, n1, n0); /* i.e., redcify(2) */
  y1 = z1 = x1;
  y0 = z0 = x0;

  while (n1 != 0 || n0 != 1)
    {
      binv (ni, n0);

      for (;;)
        {
          do
            {
              x0 = mulredc2 (&r1m, x1, x0, x1, x0, n1, n0, ni);
              x1 = r1m;
              addmod2 (x1, x0, x1, x0, 0, (wide_uint) a, n1, n0);

              submod2 (t1, t0, z1, z0, x1, x0, n1, n0);
              P0 = mulredc2 (&r1m, P1, P0, t1, t0, n1, n0, ni);
              P1 = r1m;

              if (k % 32 == 1)
                {
                  uuset (&g1, &g0, gcd2_odd (P1, P0, n1, n0));
                  if (g1 != 0 || g0 != 1)
                    goto factor_found;
                  y1 = x1; y0 = x0;
                }
            }
          while (--k != 0);

          z1 = x1; z0 = x0;
          k = l;
          l = 2 * l;
          for (unsigned long int i = 0; i < k; i++)
            {
              x0 = mulredc2 (&r1m, x1, x0, x1, x0, n1, n0, ni);
              x1 = r1m;
              addmod2 (x1, x0, x1, x0, 0, (wide_uint) a, n1, n0);
            }
          y1 = x1; y0 = x0;
        }

    factor_found:
      do
        {
          y0 = mulredc2 (&r1m, y1, y0, y1, y0, n1, n0, ni);
          y1 = r1m;
          addmod2 (y1, y0, y1, y0, 0, (wide_uint) a, n1, n0);

          submod2 (t1, t0, z1, z0, y1, y0, n1, n0);
          uuset (&g1, &g0, gcd2_odd (t1, t0, n1, n0));
        }
      while (g1 == 0 && g0 == 1);

      if (g1 == 0)
        {
          /* The found factor is one word, and > 1.  */
          divexact_21 (n1, n0, n1, n0, g0);     /* n = n / g */

          if (!prime_p (g0))
            factor_using_pollard_rho (g0, a + 1, factors);
          else
            factor_insert (factors, g0);
        }
      else
        {
          /* The found factor is two words.  This is highly unlikely, thus hard
             to trigger.  Please be careful before you change this code!  */
          wide_uint ginv;

          if (n1 == g1 && n0 == g0)
            {
              /* Found n itself as factor.  Restart with different params.  */
              factor_using_pollard_rho2 (n1, n0, a + 1, factors);
              return;
            }

          /* Compute n = n / g.  Since the result will fit one word,
             we can compute the quotient modulo B, ignoring the high
             divisor word.  */
          binv (ginv, g0);
          n0 = ginv * n0;
          n1 = 0;

          if (!prime2_p (g1, g0))
            factor_using_pollard_rho2 (g1, g0, a + 1, factors);
          else
            factor_insert_large (factors, g1, g0);
        }

      if (n1 == 0)
        {
          if (prime_p (n0))
            {
              factor_insert (factors, n0);
              break;
            }

          factor_using_pollard_rho (n0, a, factors);
          return;
        }

      if (prime2_p (n1, n0))
        {
          factor_insert_large (factors, n1, n0);
          break;
        }

      uuset (&x1, &x0, mod2 (x1, x0, n1, n0));
      uuset (&z1, &z0, mod2 (z1, z0, n1, n0));
      uuset (&y1, &y0, mod2 (y1, y0, n1, n0));
    }
}

static void
mp_factor_using_pollard_rho (mpz_t n, unsigned long int a,
                             struct mp_factors *factors)
{
  mpz_t x, z, y, P;
  mpz_t t, t2;

  devmsg ("[pollard-rho (%lu)] ", a);

  mpz_inits (t, t2, nullptr);
  mpz_init_set_si (y, 2);
  mpz_init_set_si (x, 2);
  mpz_init_set_si (z, 2);
  mpz_init_set_ui (P, 1);

  unsigned long long int k = 1;
  unsigned long long int l = 1;

  while (mpz_cmp_ui (n, 1) != 0)
    {
      for (;;)
        {
          do
            {
              mpz_mul (t, x, x);
              mpz_mod (x, t, n);
              mpz_add_ui (x, x, a);

              mpz_sub (t, z, x);
              mpz_mul (t2, P, t);
              mpz_mod (P, t2, n);

              if (k % 32 == 1)
                {
                  mpz_gcd (t, P, n);
                  if (mpz_cmp_ui (t, 1) != 0)
                    goto factor_found;
                  mpz_set (y, x);
                }
            }
          while (--k != 0);

          mpz_set (z, x);
          k = l;
          l = 2 * l;
          for (unsigned long long int i = 0; i < k; i++)
            {
              mpz_mul (t, x, x);
              mpz_mod (x, t, n);
              mpz_add_ui (x, x, a);
            }
          mpz_set (y, x);
        }

    factor_found:
      do
        {
          mpz_mul (t, y, y);
          mpz_mod (y, t, n);
          mpz_add_ui (y, y, a);

          mpz_sub (t, z, y);
          mpz_gcd (t, t, n);
        }
      while (mpz_cmp_ui (t, 1) == 0);

      mpz_divexact (n, n, t);   /* divide by t, before t is overwritten */

      if (!mp_prime_p (t))
        {
          devmsg ("[composite factor--restarting pollard-rho] ");
          mp_factor_using_pollard_rho (t, a + 1, factors);
        }
      else
        {
          mp_factor_insert (factors, t);
        }

      if (mp_prime_p (n))
        {
          mp_factor_insert (factors, n);
          break;
        }

      mpz_mod (x, x, n);
      mpz_mod (z, z, n);
      mpz_mod (y, y, n);
    }

  mpz_clears (P, t2, t, z, x, y, nullptr);
}

/* Compute the prime factors of the 128-bit number (T1,T0), and put the
   results in FACTORS.  */
static void
factor (wide_uint t1, wide_uint t0, struct factors *factors)
{
  factors->nfactors = 0;
  hiset (&factors->plarge, 0);

  if (t1 == 0 && t0 < 2)
    return;

  uuset (&t1, &t0, factor_using_division (t1, t0, factors));

  if (t1 == 0 && t0 < 2)
    return;

  if (prime2_p (t1, t0))
    factor_insert_large (factors, t1, t0);
  else
    {
      if (t1 == 0)
        factor_using_pollard_rho (t0, 1, factors);
      else
        factor_using_pollard_rho2 (t1, t0, 1, factors);
    }
}

/* Use Pollard-rho to compute the prime factors of
   arbitrary-precision T, and put the results in FACTORS.  */
static void
mp_factor (mpz_t t, struct mp_factors *factors)
{
  mp_factor_init (factors);

  if (mpz_sgn (t) != 0)
    {
      mp_factor_using_division (t, factors);

      if (mpz_cmp_ui (t, 1) != 0)
        {
          devmsg ("[is number prime?] ");
          if (mp_prime_p (t))
            mp_factor_insert (factors, t);
          else
            mp_factor_using_pollard_rho (t, 1, factors);
        }
    }
}

static strtol_error
strto2wide_uint (wide_uint *hip, wide_uint *lop, char const *s)
{
  int lo_carry;
  wide_uint hi = 0, lo = 0;

  strtol_error err = LONGINT_INVALID;

  /* Initial scan for invalid digits.  */
  char const *p = s;
  for (;;)
    {
      unsigned char c = *p++;
      if (c == 0)
        break;

      if (UNLIKELY (!c_isdigit (c)))
        {
          err = LONGINT_INVALID;
          break;
        }

      err = LONGINT_OK;           /* we've seen at least one valid digit */
    }

  while (err == LONGINT_OK)
    {
      unsigned char c = *s++;
      if (c == 0)
        break;

      c -= '0';

      if (UNLIKELY (ckd_mul (&hi, hi, 10)))
        {
          err = LONGINT_OVERFLOW;
          break;
        }

      lo_carry = (lo >> (W_TYPE_SIZE - 3)) + (lo >> (W_TYPE_SIZE - 1));
      lo_carry += 10 * lo < 2 * lo;

      lo = 10 * lo;
      lo_carry += ckd_add (&lo, lo, c);
      if (UNLIKELY (ckd_add (&hi, hi, lo_carry)))
        {
          err = LONGINT_OVERFLOW;
          break;
        }
    }

  *hip = hi;
  *lop = lo;

  return err;
}

/* FACTOR_PIPE_BUF is chosen to give good performance,
   and also is the max guaranteed size that
   consumers can read atomically through pipes.
   Also it's big enough to cater for max line length
   even with 128 bit wide_uint.  */
#ifndef _POSIX_PIPE_BUF
# define _POSIX_PIPE_BUF 512
#endif
#ifdef PIPE_BUF
enum { FACTOR_PIPE_BUF = PIPE_BUF };
#else
enum { FACTOR_PIPE_BUF = _POSIX_PIPE_BUF };
#endif

/* Structure and routines for buffering and outputting full lines, to
   support parallel operation efficiently.

   The buffer is twice FACTOR_PIPE_BUF so that its second half can
   hold the remainder of data that is somewhat too large.  Also, the
   very end of the second half is used to hold temporary data when
   stringifying integers, which is most conveniently done
   right-to-left.

   Although the buffer's second half doesn't need to be quite so large
   - its necessary size is bounded above by roughly the maximum output
   line for a uuint plus the string length of a uuint - it'd be a bit
   of a pain to figure out exactly how small it can be without causing
   trouble.  */
static char lbuf_buf[2 * FACTOR_PIPE_BUF];
static idx_t lbuffered;

/* Write complete LBUF to standard output.  */
static void
lbuf_flush (void)
{
  idx_t size = lbuffered;

  /* Update lbuffered now, to avoid infinite recursion on write error.  */
  lbuffered = 0;

  if (full_write (STDOUT_FILENO, lbuf_buf, size) != size)
    write_error ();
}

/* Write LBUF to standard output.
   LBUF should contain at least FACTOR_PIPE_BUF bytes.
   If possible, write a prefix of LBUF that is newline terminated
   and contains <= FACTOR_PIPE_BUF bytes, so consumers can read atomically.
   But if the first FACTOR_PIPE_BUF bytes contain no newlines,
   give up on atomicity and just write the first FACTOR_PIPE_BUF bytes.  */
static void
lbuf_half_flush (void)
{
  char *nl = memrchr (lbuf_buf, '\n', FACTOR_PIPE_BUF);
  char *suffix = nl ? nl + 1 : lbuf_buf + FACTOR_PIPE_BUF;
  idx_t prefix_size = suffix - lbuf_buf;
  idx_t suffix_size = lbuffered - prefix_size;
  lbuffered = prefix_size;
  lbuf_flush ();
  lbuffered = suffix_size;
  memmove (lbuf_buf, suffix, suffix_size);
}

/* Add a character C to lbuf_buf.  */
static void
lbuf_putc (char c)
{
  lbuf_buf[lbuffered++] = c;
}

/* Add a newline to lbuf_buf.  Then, if enough bytes are already
   buffered, write the buffer atomically to standard output.  */
static void
lbuf_putnl (void)
{
  lbuf_putc ('\n');

  /* Provide immediate output for interactive use.  */
  static int line_buffered = -1;
  if (line_buffered < 0)
    line_buffered = isatty (STDOUT_FILENO);

  if (line_buffered)
    lbuf_flush ();
  else if (FACTOR_PIPE_BUF <= lbuffered)
    lbuf_half_flush ();
}

/* Append the string representation of I to lbuf_buf, followed by
   everything from BUFEND to lbuf_buf's end.  Use the area just before
   BUFEND temporarily.  */
static void
lbuf_putint_append (wide_uint i, char *bufend)
{
  char *istr = bufend;
  do
    {
      *--istr = '0' + i % 10;
      i /= 10;
    }
  while (i);

  char *p = lbuf_buf + lbuffered;
  do
    *p++ = *istr++;
  while (istr < lbuf_buf + sizeof lbuf_buf);

  lbuffered = p - lbuf_buf;
}

/* Append the string representation of I to lbuf_buf.  */
static void
lbuf_putint (wide_uint i)
{
  lbuf_putint_append (i, lbuf_buf + sizeof lbuf_buf);
}

/* Append the string representation of T to lbuf_buf.  */
static void
print_uuint (uuint t)
{
  wide_uint t1 = hi (t), t0 = lo (t);
  char *bufend = lbuf_buf + sizeof lbuf_buf;

  while (t1)
    {
      wide_uint r = t1 % BIG_POWER_OF_10;
      t1 /= BIG_POWER_OF_10;
      udiv_qrnnd (t0, r, r, t0, BIG_POWER_OF_10);
      for (int i = 0; i < LOG_BIG_POWER_OF_10; i++)
        {
          *--bufend = '0' + r % 10;
          r /= 10;
        }
    }

  lbuf_putint_append (t0, bufend);
}

/* Buffer an mpz to the internal LBUF, possibly writing if it is long.  */
static void
lbuf_putmpz (mpz_t const i)
{
  idx_t sizeinbase = mpz_sizeinbase (i, 10);
  char *lbuf_bufend = lbuf_buf + sizeof lbuf_buf;
  char *p = lbuf_buf + lbuffered;
  if (sizeinbase < lbuf_bufend - p)
    {
      mpz_get_str (p, 10, i);
      p += sizeinbase;
      lbuffered = p - !p[-1] - lbuf_buf;
      while (FACTOR_PIPE_BUF <= lbuffered)
        lbuf_half_flush ();
    }
  else
    {
      lbuf_flush ();
      char *istr = ximalloc (sizeinbase + 1);
      mpz_get_str (istr, 10, i);
      idx_t istrlen = sizeinbase - !istr[sizeinbase - 1];
      if (full_write (STDOUT_FILENO, istr, istrlen) != istrlen)
        write_error ();
      free (istr);
    }
}

/* Single-precision factoring */
static void
print_factors_single (wide_uint t1, wide_uint t0)
{
  struct factors factors;

  print_uuint (make_uuint (t1, t0));
  lbuf_putc (':');

  factor (t1, t0, &factors);

  for (int j = 0; j < factors.nfactors; j++)
    for (int k = 0; k < factors.e[j]; k++)
      {
        lbuf_putc (' ');
        print_uuint (make_uuint (0, factors.p[j]));
        if (print_exponents && factors.e[j] > 1)
          {
            lbuf_putc ('^');
            lbuf_putint (factors.e[j]);
            break;
          }
      }

  if (hi_is_set (&factors.plarge))
    {
      lbuf_putc (' ');
      print_uuint (factors.plarge);
    }

  lbuf_putnl ();
}

/* Emit the factors of the indicated number.  If we have the option of using
   either algorithm, we select on the basis of the length of the number.
   For longer numbers, we prefer the MP algorithm even if the native algorithm
   has enough digits, because the algorithm is better.  The turnover point
   depends on the value.  */
static bool
print_factors (char const *input)
{
  /* Skip initial spaces and '+'.  */
  char const *str = input;
  while (*str == ' ')
    str++;
  str += *str == '+';

  wide_uint t1, t0;

  /* Try converting the number to one or two words.  If it fails, use GMP or
     print an error message.  The 2nd condition checks that the most
     significant bit of the two-word number is clear, in a typesize neutral
     way.  */
  strtol_error err = strto2wide_uint (&t1, &t0, str);

  switch (err)
    {
    case LONGINT_OK:
      if (((t1 << 1) >> 1) == t1)
        {
          devmsg ("[using single-precision arithmetic] ");
          print_factors_single (t1, t0);
          return true;
        }
      break;

    case LONGINT_OVERFLOW:
      /* Try GMP.  */
      break;

    case LONGINT_INVALID:
    case LONGINT_INVALID_SUFFIX_CHAR:
    case LONGINT_INVALID_SUFFIX_CHAR_WITH_OVERFLOW:
    default:
      error (0, 0, _("%s is not a valid positive integer"), quote (input));
      return false;
    }

  devmsg ("[using arbitrary-precision arithmetic] ");
  mpz_t t;
  struct mp_factors factors;

  mpz_init_set_str (t, str, 10);

  lbuf_putmpz (t);
  lbuf_putc (':');
  mp_factor (t, &factors);

  for (idx_t j = 0; j < factors.nfactors; j++)
    for (unsigned long int k = 0; k < factors.e[j]; k++)
      {
        lbuf_putc (' ');
        lbuf_putmpz (factors.p[j]);
        if (print_exponents && factors.e[j] > 1)
          {
            lbuf_putc ('^');
            lbuf_putint (factors.e[j]);
            break;
          }
      }

  mp_factor_clear (&factors);
  mpz_clear (t);
  lbuf_putnl ();
  return true;
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION] [NUMBER]...\n\
"),
              program_name);
      fputs (_("\
Print the prime factors of each specified integer NUMBER.  If none\n\
are specified on the command line, read them from standard input.\n\
\n\
"), stdout);
      fputs ("\
  -h, --exponents   print repeated factors in form p^e unless e is 1\n\
", stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

static bool
do_stdin (void)
{
  bool ok = true;
  token_buffer tokenbuffer;

  init_tokenbuffer (&tokenbuffer);

  while (true)
    {
      size_t token_length = readtoken (stdin, DELIM, sizeof (DELIM) - 1,
                                       &tokenbuffer);
      if (token_length == (size_t) -1)
        {
          if (ferror (stdin))
            error (EXIT_FAILURE, errno, _("error reading input"));
          break;
        }

      ok &= print_factors (tokenbuffer.buffer);
    }
  free (tokenbuffer.buffer);

  return ok;
}

int
main (int argc, char **argv)
{
  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  int c;
  while ((c = getopt_long (argc, argv, "h", long_options, nullptr)) != -1)
    {
      switch (c)
        {
        case 'h':  /* NetBSD used -h for this functionality first.  */
          print_exponents = true;
          break;

        case DEV_DEBUG_OPTION:
          dev_debug = true;
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }
    }

  atexit (lbuf_flush);

  bool ok;
  if (argc <= optind)
    ok = do_stdin ();
  else
    {
      ok = true;
      for (int i = optind; i < argc; i++)
        if (! print_factors (argv[i]))
          ok = false;
    }

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
