/* xstrtod.c - error-checking interface to strtod
   Copyright (C) 1996, 1999, 2000 Free Software Foundation, Inc.

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

/* Written by Jim Meyering.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
double strtod ();
#endif

#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include "xstrtod.h"

/* Tell the compiler that non-default rounding modes are used.  */
#if 199901 <= __STDC_VERSION__
 #pragma STDC FENV_ACCESS ON
#endif

/* An interface to strtod that encapsulates all the error checking
   one should usually perform.  Like strtod, but upon successful
   conversion put the result in *RESULT and return zero.  Return
   non-zero and don't modify *RESULT upon any failure.  */

int
xstrtod (str, ptr, result)
     const char *str;
     const char **ptr;
     double *result;
{
  double val;
  char *terminator;
  int fail;

  fail = 0;
  errno = 0;
  val = strtod (str, &terminator);

  /* Having a non-zero terminator is an error only when PTR is NULL. */
  if (terminator == str || (ptr == NULL && *terminator != '\0'))
    fail = 1;
  else
    {
      /* Allow underflow (in which case strtod returns zero),
	 but flag overflow as an error. */
      if (val != 0.0 && errno == ERANGE)
	fail = 1;
    }

  if (ptr != NULL)
    *ptr = terminator;

  *result = val;
  return fail;
}
