/* xstrtoumax.c -- A more useful interface to strtoumax.
   Copyright 1999 Free Software Foundation, Inc.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Paul Eggert. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_STRTOUMAX
# define __strtol strtoumax
#else
# define __strtol my_strtoumax

  /* Define a temporary implementation, intended to hold the fort only
     until glibc has an implementation of the C9x standard function
     strtoumax.  When glibc implements strtoumax, we can remove the
     following function in favor of a replacement file strtoumax.c taken
     from glibc.  */

# if HAVE_INTTYPES_H
#  include <inttypes.h>
# endif

# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif

# ifndef PARAMS
#  if defined PROTOTYPES || defined __STDC__
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

static uintmax_t my_strtoumax PARAMS ((char const *, char **, int));
#endif

#define __strtol_t uintmax_t
#define __xstrtol xstrtoumax
#include "xstrtol.c"

#if !HAVE_STRTOUMAX

static uintmax_t
my_strtoumax (char const *ptr, char **endptr, int base)
{
# define USE_IF_EQUIVALENT(function) \
    if (sizeof (uintmax_t) == sizeof function (ptr, endptr, base)) \
      return function (ptr, endptr, base);

  USE_IF_EQUIVALENT (strtoul)

# if HAVE_STRTOULL
    USE_IF_EQUIVALENT (strtoull)
# endif

# if HAVE_STRTOUQ
    USE_IF_EQUIVALENT (strtouq)
# endif

  {
    /* An implementation with uintmax_t longer than long, but with no
       known way to convert it.  Do it by hand.  Assume base 10.  */
    uintmax_t n = 0;
    uintmax_t overflow = 0;
    for (;  '0' <= *ptr && *ptr <= '9';  ptr++)
      {
	uintmax_t n10 = n * 10;
	int digit = *ptr - '0';
	overflow |= n ^ (n10 + digit) / 10;
	n = n10 + digit;
      }
    if (endptr)
      *endptr = (char *) ptr;
    if (overflow)
      {
	errno = ERANGE;
	n = (uintmax_t) -1;
      }
    return n;
  }
}
#endif /* HAVE_STRTOUMAX */
