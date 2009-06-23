/* Bob Jenkins's cryptographic random number generator, ISAAC.

   Copyright (C) 1999-2005, 2009 Free Software Foundation, Inc.
   Copyright (C) 1997, 1998, 1999 Colin Plumb.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Written by Colin Plumb.  */

#ifndef RAND_ISAAC_H
# define RAND_ISAAC_H

# include <stdint.h>

/* Size of the state tables to use.  ISAAC_LOG should be at least 3,
   and smaller values give less security.  */
# define ISAAC_LOG 8
# define ISAAC_WORDS (1 << ISAAC_LOG)
# define ISAAC_BYTES (ISAAC_WORDS * sizeof (uint32_t))

/* RNG state variables.  The members of this structure are private.  */
struct isaac_state
  {
    uint32_t mm[ISAAC_WORDS];	/* Main state array */
    uint32_t iv[8];		/* Seeding initial vector */
    uint32_t a, b, c;		/* Extra index variables */
  };

void isaac_seed (struct isaac_state *);
void isaac_refill (struct isaac_state *, uint32_t[ISAAC_WORDS]);

#endif
