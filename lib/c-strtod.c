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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

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
#else
# define C_STRTOD c_strtod
# define DOUBLE double
#endif

/* c_strtold falls back on strtod if strtold isn't declared.  */
#if LONG && HAVE_DECL_STRTOLD
# define STRTOD strtold
#else
# define STRTOD strtod
#endif

DOUBLE
C_STRTOD (char const *nptr, char **endptr)
{
  DOUBLE r;
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

  return r;
}
