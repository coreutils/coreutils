/* rand-isaac.c - ISAAC random number generator

   Copyright (C) 1999-2005 Free Software Foundation, Inc.
   Copyright (C) 1997, 1998, 1999 Colin Plumb.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Written by Colin Plumb.  */

/*
 * --------------------------------------------------------------------
 *     Bob Jenkins' cryptographic random number generator, ISAAC.
 *     Hacked by Colin Plumb.
 *
 * We need a source of random numbers for some of the overwrite data
 * for shred. Cryptographically secure is desirable, but it's not
 * life-or-death so I can be a little bit experimental in the choice
 * of RNGs here.
 *
 * This generator is based somewhat on RC4, but has analysis
 * <http://burtleburtle.net/bob/rand/isaacafa.html>
 * pointing to it actually being better.  I like it because it's nice
 * and fast, and because the author did good work analyzing it.
 * --------------------------------------------------------------------
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "system.h"
#include "gethrxtime.h"

#include "rand-isaac.h"

#define ISAAC_SIZE(words) \
    (sizeof (struct isaac_state) - \
    (ISAAC_MAX_WORDS - words) * sizeof (uint32_t))

/*
 * Create a new state, optionally at the location specified. This
 * should be called to create/initialize each new isaac_state. 'words'
 * must be a power of 2, and should be at least 8. Smaller values give
 * less security.
 */
extern struct isaac_state *
isaac_new (struct isaac_state *s, int words)
{
  size_t w;
  size_t ss = ISAAC_SIZE (words);
  if (!s)
    {
      s = xmalloc (ss);
    }
  memset (s, 0, ss);
  s->words = words;
  s->log = 0;
  for (w=1; w<words; w<<=1, s->log++) {}
  return s;
}

/*
 * Copy a state. Destination block must be at least as large as
 * source.
 */
extern void
isaac_copy (struct isaac_state *dst, struct isaac_state *src)
{
  memcpy(dst, src, ISAAC_SIZE (src->words));
}

/*
 * Refill the entire R array, and update S.
 */
extern void
isaac_refill (struct isaac_state *s, uint32_t r[/* s>-words */])
{
  uint32_t a, b;		/* Caches of a and b */
  uint32_t x, y;		/* Temps needed by isaac_step macro */
  uint32_t *m = s->mm;	/* Pointer into state array */
  int w = s->words;

  a = s->a;
  b = s->b + (++s->c);

  do
    {
      isaac_step (s, a << 13, a, b, m, w / 2, r);
      isaac_step (s, a >> 6, a, b, m + 1, w / 2, r + 1);
      isaac_step (s, a << 2, a, b, m + 2, w / 2, r + 2);
      isaac_step (s, a >> 16, a, b, m + 3, w / 2, r + 3);
      r += 4;
    }
  while ((m += 4) < s->mm + w / 2);

  do
    {
      isaac_step (s, a << 13, a, b, m, -w / 2, r);
      isaac_step (s, a >> 6, a, b, m + 1, -w / 2, r + 1);
      isaac_step (s, a << 2, a, b, m + 2, -w / 2, r + 2);
      isaac_step (s, a >> 16, a, b, m + 3, -w / 2, r + 3);
      r += 4;
    }
  while ((m += 4) < s->mm + w);

  s->a = a;
  s->b = b;
}

/*
 * The basic seed-scrambling step for initialization, based on Bob
 * Jenkins' 256-bit hash.
 */
#define mix(a,b,c,d,e,f,g,h) \
   (       a ^= b << 11, d += a, \
   b += c, b ^= c >>  2, e += b, \
   c += d, c ^= d <<  8, f += c, \
   d += e, d ^= e >> 16, g += d, \
   e += f, e ^= f << 10, h += e, \
   f += g, f ^= g >>  4, a += f, \
   g += h, g ^= h <<  8, b += g, \
   h += a, h ^= a >>  9, c += h, \
   a += b                        )

/* The basic ISAAC initialization pass.  */
static void
isaac_mix (struct isaac_state *s, uint32_t const seed[/* s->words */])
{
  int i;
  uint32_t a = s->iv[0];
  uint32_t b = s->iv[1];
  uint32_t c = s->iv[2];
  uint32_t d = s->iv[3];
  uint32_t e = s->iv[4];
  uint32_t f = s->iv[5];
  uint32_t g = s->iv[6];
  uint32_t h = s->iv[7];
  uint32_t w = s->words;

  for (i = 0; i < w; i += 8)
    {
      a += seed[i];
      b += seed[i + 1];
      c += seed[i + 2];
      d += seed[i + 3];
      e += seed[i + 4];
      f += seed[i + 5];
      g += seed[i + 6];
      h += seed[i + 7];

      mix (a, b, c, d, e, f, g, h);

      s->mm[i] = a;
      s->mm[i + 1] = b;
      s->mm[i + 2] = c;
      s->mm[i + 3] = d;
      s->mm[i + 4] = e;
      s->mm[i + 5] = f;
      s->mm[i + 6] = g;
      s->mm[i + 7] = h;
    }

  s->iv[0] = a;
  s->iv[1] = b;
  s->iv[2] = c;
  s->iv[3] = d;
  s->iv[4] = e;
  s->iv[5] = f;
  s->iv[6] = g;
  s->iv[7] = h;
}

#if 0 /* Provided for reference only; not used in this code */
/*
 * Initialize the ISAAC RNG with the given seed material. Its size
 * MUST be a multiple of s->words * sizeof (uint32_t), and may be
 * stored in the s->mm array.
 *
 * This is a generalization of the original ISAAC initialization code
 * to support larger seed sizes. For seed sizes of 0 and s->words *
 * sizeof (uint32_t), it is identical.
 */
extern void
isaac_init (struct isaac_state *s, uint32_t const *seed, size_t seedsize)
{
  static uint32_t const iv[8] =
  {
    0x1367df5a, 0x95d90059, 0xc3163e4b, 0x0f421ad8,
    0xd92a4a78, 0xa51a3c49, 0xc4efea1b, 0x30609119};
  int i;

# if 0
  /* The initialization of iv is a precomputed form of: */
  for (i = 0; i < 7; i++)
    iv[i] = 0x9e3779b9;		/* the golden ratio */
  for (i = 0; i < 4; ++i)	/* scramble it */
    mix (iv[0], iv[1], iv[2], iv[3], iv[4], iv[5], iv[6], iv[7]);
# endif
  s->a = s->b = s->c = 0;

  for (i = 0; i < 8; i++)
    s->iv[i] = iv[i];

  if (seedsize)
    {
      /* First pass (as in reference ISAAC code) */
      isaac_mix (s, seed);
      /* Second and subsequent passes (extension to ISAAC) */
      while (seedsize -= s->words * sizeof (uint32_t))
	{
	  seed += s->words;
	  for (i = 0; i < s->words; i++)
	    s->mm[i] += seed[i];
	  isaac_mix (s, s->mm);
	}
    }
  else
    {
      /* The no seed case (as in reference ISAAC code) */
      for (i = 0; i < s->words; i++)
	s->mm[i] = 0;
    }

  /* Final pass */
  isaac_mix (s, s->mm);
}
#endif

/* Start seeding an ISAAC structire */
extern void
isaac_seed_start (struct isaac_state *s)
{
  static uint32_t const iv[8] =
    {
      0x1367df5a, 0x95d90059, 0xc3163e4b, 0x0f421ad8,
      0xd92a4a78, 0xa51a3c49, 0xc4efea1b, 0x30609119
    };
  int i;

#if 0
  /* The initialization of iv is a precomputed form of: */
  for (i = 0; i < 7; i++)
    iv[i] = 0x9e3779b9;		/* the golden ratio */
  for (i = 0; i < 4; ++i)	/* scramble it */
    mix (iv[0], iv[1], iv[2], iv[3], iv[4], iv[5], iv[6], iv[7]);
#endif
  for (i = 0; i < 8; i++)
    s->iv[i] = iv[i];

  /* used to do memset here to clear the state array, but now it's
     done in isaac_new */

  /* s->c gets used for a data pointer during the seeding phase */
  s->a = s->b = s->c = 0;
}

/* Add a buffer of seed material */
extern void
isaac_seed_data (struct isaac_state *s, void const *buffer, size_t size)
{
  unsigned char const *buf = buffer;
  unsigned char *p;
  size_t avail;
  size_t i;

  avail = s->words - (size_t) s->c;	/* s->c is used as a write pointer */

  /* Do any full buffers that are necessary */
  while (size > avail)
    {
      p = (unsigned char *) s->mm + s->c;
      for (i = 0; i < avail; i++)
	p[i] ^= buf[i];
      buf += avail;
      size -= avail;
      isaac_mix (s, s->mm);
      s->c = 0;
      avail = s->words;
    }

  /* And the final partial block */
  p = (unsigned char *) s->mm + s->c;
  for (i = 0; i < size; i++)
    p[i] ^= buf[i];
  s->c = size;
}


/* End of seeding phase; get everything ready to produce output. */
extern void
isaac_seed_finish (struct isaac_state *s)
{
  isaac_mix (s, s->mm);
  isaac_mix (s, s->mm);
  /* Now reinitialize c to start things off right */
  s->c = 0;
}
#define ISAAC_SEED(s,x) isaac_seed_data (s, &(x), sizeof (x))

/*
 * Get seed material.  16 bytes (128 bits) is plenty, but if we have
 * /dev/urandom, we get 32 bytes = 256 bits for complete overkill.
 */
extern void
isaac_seed (struct isaac_state *s)
{
  isaac_seed_start (s);

  { pid_t t = getpid ();   ISAAC_SEED (s, t); }
  { pid_t t = getppid ();  ISAAC_SEED (s, t); }
  { uid_t t = getuid ();   ISAAC_SEED (s, t); }
  { gid_t t = getgid ();   ISAAC_SEED (s, t); }

  {
    xtime_t t = gethrxtime ();
    ISAAC_SEED (s, t);
  }

  {
    char buf[32];
    int fd = open ("/dev/urandom", O_RDONLY | O_NOCTTY);
    if (fd >= 0)
      {
	read (fd, buf, 32);
	close (fd);
	isaac_seed_data (s, buf, 32);
      }
    else
      {
	fd = open ("/dev/random", O_RDONLY | O_NONBLOCK | O_NOCTTY);
	if (fd >= 0)
	  {
	    /* /dev/random is more precious, so use less */
	    read (fd, buf, 16);
	    close (fd);
	    isaac_seed_data (s, buf, 16);
	  }
      }
  }

  isaac_seed_finish (s);
}

extern void
irand_init (struct irand_state *r, struct isaac_state *s)
{
  r->numleft = 0;
  r->s = s;
  memset (r->r, 0, s->words * sizeof (uint32_t));
}

/*
 * We take from the end of the block deliberately, so if we need
 * only a small number of values, we choose the final ones which are
 * marginally better mixed than the initial ones.
 */
extern uint32_t
irand32 (struct irand_state *r)
{
  if (!r->numleft)
    {
      isaac_refill (r->s, r->r);
      r->numleft = r->s->words;
    }
  return r->r[--r->numleft];
}

/*
 * Return a uniformly distributed random number between 0 and n,
 * inclusive.  Thus, the result is modulo n+1.
 *
 * Theory of operation: as x steps through every possible 32-bit number,
 * x % n takes each value at least 2^32 / n times (rounded down), but
 * the values less than 2^32 % n are taken one additional time.  Thus,
 * x % n is not perfectly uniform.  To fix this, the values of x less
 * than 2^32 % n are disallowed, and if the RNG produces one, we ask
 * for a new value.
 */
extern uint32_t
irand_mod (struct irand_state *r, uint32_t n)
{
  uint32_t x;
  uint32_t lim;

  if (!++n)
    return irand32 (r);

  lim = -n % n;			/* == (2**32-n) % n == 2**32 % n */
  do
    {
      x = irand32 (r);
    }
  while (x < lim);
  return x % n;
}
