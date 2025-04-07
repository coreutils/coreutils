/* Compute a timespec-related bound for floating point.

   Copyright 2025 Free Software Foundation, Inc.

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

/* written by Paul Eggert */

#ifndef DTIMESPEC_BOUND_H
# define DTIMESPEC_BOUND_H 1

/* This file uses _GL_INLINE_HEADER_BEGIN, _GL_INLINE.  */
# if !_GL_CONFIG_H_INCLUDED
#  error "Please include config.h first."
# endif

# include <errno.h>
# include <float.h>
# include <math.h>

_GL_INLINE_HEADER_BEGIN
# ifndef DTIMESPEC_BOUND_INLINE
#  define DTIMESPEC_BOUND_INLINE _GL_INLINE
# endif

/* If C is positive and finite, return the least floating point value
   greater than C.  However, if 0 < C < (2 * DBL_TRUE_MIN) / (DBL_EPSILON**2),
   return a positive value less than 1e-9.

   If C is +0.0, return a positive value < 1e-9 if ERR == ERANGE, C otherwise.
   If C is +Inf, return C.
   If C is negative, return -timespec_roundup(-C).
   If C is a NaN, return a NaN.

   Assume round-to-even.

   This function can be useful if some floating point operation
   rounded to C but we want a nearby bound on the true value, where
   the bound can be converted to struct timespec.  If the operation
   underflowed to zero, ERR should be ERANGE a la strtod.  */

DTIMESPEC_BOUND_INLINE double
dtimespec_bound (double c, int err)
{
  /* Do not use copysign or nextafter, as they link to -lm in GNU/Linux.  */

  /* Use DBL_TRUE_MIN for the special case of underflowing to zero;
     any positive value less than 1e-9 will do.  */
  if (err == ERANGE && c == 0)
    return signbit (c) ? -DBL_TRUE_MIN : DBL_TRUE_MIN;

  /* This is the first part of Algorithm 2 of:
     Rump SM, Zimmermann P, Boldo S, Melquiond G.
     Computing predecessor and successor in rounding to nearest.
     BIT Numerical Mathematics. 2009;49(419-431).
     <https://doi.org/10.1007/s10543-009-0218-z>
     The rest of Algorithm 2 is not needed because numbers less than
     the predecessor of 1e-9 merely need to stay less than 1e-9.  */
  double phi = DBL_EPSILON / 2 * (1 + DBL_EPSILON);
  return c + phi * c;
}

_GL_INLINE_HEADER_END

#endif
