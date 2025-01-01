/* Convert string to double in the current locale, falling back on the C locale.

   Copyright 2019-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Paul Eggert.  */

#include <config.h>

#include "cl-strtod.h"

#include <c-strtod.h>

#include <errno.h>
#include <stdlib.h>

#if LONG
# define CL_STRTOD cl_strtold
# define DOUBLE long double
# define C_STRTOD c_strtold
# define STRTOD strtold
#else
# define CL_STRTOD cl_strtod
# define DOUBLE double
# define C_STRTOD c_strtod
# define STRTOD strtod
#endif

/* This function acts like strtod or strtold, except that it falls
   back on the C locale if the initial prefix is not parsable in
   the current locale.  If the prefix is parsable in both locales,
   it uses the longer parse, breaking ties in favor of the current locale.

   Parse the initial prefix of NPTR as a floating-point number in the
   current locale or in the C locale (preferring the locale that
   yields the longer parse, or the current locale if there is a tie).
   If ENDPTR is non-null, set *ENDPTR to the first unused byte, or to
   NPTR if the prefix cannot be parsed.

   If successful, return a number without changing errno.
   If the prefix cannot be parsed, return 0 and possibly set errno to EINVAL.
   If the number overflows, return an extreme value and set errno to ERANGE.
   If the number underflows, return a value close to 0 and set errno to ERANGE.
   If there is some other error, return 0 and set errno.  */

DOUBLE
CL_STRTOD (char const *nptr, char **restrict endptr)
{
  char *end;
  DOUBLE d = STRTOD (nptr, &end);
  if (*end)
    {
      int strtod_errno = errno;
      char *c_end;
      DOUBLE c = C_STRTOD (nptr, &c_end);
      if (end < c_end)
        d = c, end = c_end;
      else
        errno = strtod_errno;
    }
  if (endptr)
    *endptr = end;
  return d;
}
