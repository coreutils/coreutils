/* rand-isaac.h - header for ISAAC

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

/* Size of the state tables to use. */
/* (You may change ISAAC_LOG but it should be >= 3, and smaller values
 * give less security) */
#ifndef ISAAC_LOG
# define ISAAC_LOG 8
#endif
#define ISAAC_MAX_WORDS (1 << ISAAC_LOG)
#define ISAAC_MAX_BYTES (ISAAC_MAX_WORDS * sizeof (uint32_t))

/* RNG state variables */
struct isaac_state
  {
    uint32_t iv[8];		    /* Seeding initial vector */
    uint32_t a, b, c;	            /* Extra index variables */
    uint32_t words;                 /* Words in main state array */
    uint32_t log;                   /* Log of words */
    uint32_t mm[ISAAC_MAX_WORDS];   /* Main state array */
  };

/* This index operation is more efficient on many processors */
#define ind(words, mm, x) \
  (* (uint32_t *) ((char *) (mm) \
	           + ((x) & (words - 1) * sizeof (uint32_t))))

/*
 * The central step. This uses two temporaries, x and y. s is the
 * state, while m is a pointer to the current word. off is the offset
 * from m to the word s->words/2 words away in the mm array, i.e.
 * +/- s->words/2.
 */
#define isaac_step(s, mix, a, b, m, off, r)  \
( \
  a = ((a) ^ (mix)) + (m)[off], \
  x = *(m), \
  *(m) = y = ind (s->words, s->mm, x) + (a) + (b),  \
  *(r) = b = ind (s->words, s->mm, (y) >> s->log) + x      \
)

struct isaac_state *
isaac_new (struct isaac_state *s, int words);
void
isaac_copy (struct isaac_state *dst, struct isaac_state *src);
void
isaac_refill (struct isaac_state *s, uint32_t r[/* s->words */]);
void
isaac_init (struct isaac_state *s, uint32_t const *seed, size_t seedsize);
void
isaac_seed_start (struct isaac_state *s);
void
isaac_seed_data (struct isaac_state *s, void const *buffer, size_t size);
void
isaac_seed_finish (struct isaac_state *s);
#define ISAAC_SEED(s,x) isaac_seed_data (s, &(x), sizeof (x))
void
isaac_seed (struct isaac_state *s);

/* Single-word RNG built on top of ISAAC */
struct irand_state
{
  uint32_t r[ISAAC_MAX_WORDS];
  unsigned int numleft;
  struct isaac_state *s;
};

void
irand_init (struct irand_state *r, struct isaac_state *s);
uint32_t
irand32 (struct irand_state *r);
uint32_t
irand_mod (struct irand_state *r, uint32_t n);
