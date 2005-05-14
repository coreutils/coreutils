/* Convert string to double, using the C locale.

   Copyright (C) 2003, 2004 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "c-strtod.h"

#include <locale.h>
#include <stdlib.h>

#include "xalloc.h"

#if LONG
# define C_STRTOD c_strtold
# define DOUBLE long double
# define STRTOD_L strtold_l
#else
# define C_STRTOD c_strtod
# define DOUBLE double
# define STRTOD_L strtod_l
#endif

/* c_strtold falls back on strtod if strtold doesn't conform to C99.  */
#if LONG && HAVE_C99_STRTOLD
# define STRTOD strtold
#else
# define STRTOD strtod
#endif

DOUBLE
C_STRTOD (char const *nptr, char **endptr)
{
  DOUBLE r;

#ifdef LC_ALL_MASK

  locale_t c_locale = newlocale (LC_ALL_MASK, "C", 0);
  r = STRTOD_L (nptr, endptr, c_locale);
  freelocale (c_locale);

#else

  char *saved_locale = setlocale (LC_NUMERIC, NULL);

  if (saved_locale)
    {
      saved_locale = xstrdup (saved_locale);
      setlocale (LC_NUMERIC, "C");
    }

  r = STRTOD (nptr, endptr);

  if (saved_locale)
    {
      setlocale (LC_NUMERIC, saved_locale);
      free (saved_locale);
    }

#endif

  return r;
}
