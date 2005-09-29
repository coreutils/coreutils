/* xtime -- extended-resolution integer time stamps

   Copyright (C) 2005 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Paul Eggert.  */

#ifndef XTIME_H_
# define XTIME_H_ 1

/* xtime_t is a signed type used for time stamps.  It is an integer
   type that is a count of nanoseconds -- except for obsolescent hosts
   without sufficiently-wide integers, where it is a count of
   seconds.  */
# if HAVE_LONG_LONG
typedef long long int xtime_t;
#  define XTIME_PRECISION 1000000000
# else
#  include <limits.h>
typedef long int xtime_t;
#  if LONG_MAX >> 31 >> 31 == 0
#   define XTIME_PRECISION 1
#  else
#   define XTIME_PRECISION 1000000000
#  endif
# endif

/* Return an extended time value that contains S seconds and NS
   nanoseconds, without any overflow checking.  */
static inline xtime_t
xtime_make (xtime_t s, int ns)
{
  if (XTIME_PRECISION == 1)
    return s;
  else
    return XTIME_PRECISION * s + ns;
}

/* Return the number of seconds in T, which must be nonnegative.  */
static inline xtime_t
xtime_nonnegative_sec (xtime_t t)
{
  return t / XTIME_PRECISION;
}

/* Return the number of seconds in T.  */
static inline xtime_t
xtime_sec (xtime_t t)
{
  return (XTIME_PRECISION == 1
	  ? t
	  : t < 0
	  ? (t + XTIME_PRECISION - 1) / XTIME_PRECISION - 1
	  : xtime_nonnegative_sec (t));
}

/* Return the number of nanoseconds in T, which must be nonnegative.  */
static inline int
xtime_nonnegative_nsec (xtime_t t)
{
  return t % XTIME_PRECISION;
}

/* Return the number of nanoseconds in T.  */
static inline int
xtime_nsec (xtime_t t)
{
  int ns = t % XTIME_PRECISION;
  if (ns < 0)
    ns += XTIME_PRECISION;
  return ns;
}

#endif
