/* Copyright (C) 1991, 1992, 1994, 1995 Free Software Foundation, Inc.


NOTE: The canonical source of this file is maintained with the GNU C Library.
Bugs can be reported to bug-glibc@prep.ai.mit.edu.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef _LIBC
# define USE_NUMBER_GROUPING
# define STDC_HEADERS
# define HAVE_LIMITS_H
#endif

#include <ctype.h>
#include <errno.h>
#ifndef errno
extern int errno;
#endif

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#ifdef STDC_HEADERS
# include <stddef.h>
# include <stdlib.h>
#else
# ifndef NULL
#  define NULL 0
# endif
#endif

#ifdef USE_NUMBER_GROUPING
# include "../locale/localeinfo.h"
#endif

/* Nonzero if we are defining `strtoul' or `strtouq', operating on
   unsigned integers.  */
#ifndef UNSIGNED
# define UNSIGNED 0
# define INT LONG int
#else
# define strtol strtoul
# define INT unsigned LONG int
#endif

/* If QUAD is defined, we are defining `strtoq' or `strtouq',
   operating on `long long int's.  */
#ifdef QUAD
# if UNSIGNED
#  define strtoul strtouq
# else
#  define strtol strtoq
# endif
# define LONG long long
# undef LONG_MIN
# define LONG_MIN LONG_LONG_MIN
# undef LONG_MAX
# define LONG_MAX LONG_LONG_MAX
# undef ULONG_MAX
# define ULONG_MAX ULONG_LONG_MAX
# if __GNUC__ == 2 && __GNUC_MINOR__ < 7
   /* Work around gcc bug with using this constant.  */
   static const unsigned long long int maxquad = ULONG_LONG_MAX;
#  undef ULONG_MAX
#  define ULONG_MAX maxquad
# endif
#else
# define LONG long
#endif

#ifdef __STDC__
# define INTERNAL(x) INTERNAL1(x)
# define INTERNAL1(x) __##x##_internal
#else
# define INTERNAL(x) __/**/x/**/_internal
#endif

#ifdef USE_NUMBER_GROUPING
/* This file defines a function to check for correct grouping.  */
# include "grouping.h"
#endif


/* Convert NPTR to an `unsigned long int' or `long int' in base BASE.
   If BASE is 0 the base is determined by the presence of a leading
   zero, indicating octal or a leading "0x" or "0X", indicating hexadecimal.
   If BASE is < 2 or > 36, it is reset to 10.
   If ENDPTR is not NULL, a pointer to the character after the last
   one converted is stored in *ENDPTR.  */

INT
INTERNAL (strtol) (nptr, endptr, base, group)
     const char *nptr;
     char **endptr;
     int base;
     int group;
{
  int negative;
  register unsigned LONG int cutoff;
  register unsigned int cutlim;
  register unsigned LONG int i;
  register const char *s;
  register unsigned char c;
  const char *save, *end;
  int overflow;

#ifdef USE_NUMBER_GROUPING
  /* The thousands character of the current locale.  */
  wchar_t thousands;
  /* The numeric grouping specification of the current locale,
     in the format described in <locale.h>.  */
  const char *grouping;

  if (group)
    {
      grouping = _NL_CURRENT (LC_NUMERIC, GROUPING);
      if (*grouping <= 0 || *grouping == CHAR_MAX)
	grouping = NULL;
      else
	{
	  /* Figure out the thousands separator character.  */
	  if (mbtowc (&thousands, _NL_CURRENT (LC_NUMERIC, THOUSANDS_SEP),
		      strlen (_NL_CURRENT (LC_NUMERIC, THOUSANDS_SEP))) <= 0)
	    thousands = (wchar_t) *_NL_CURRENT (LC_NUMERIC, THOUSANDS_SEP);
	  if (thousands == L'\0')
	    grouping = NULL;
	}
    }
  else
    grouping = NULL;
#endif

  if (base < 0 || base == 1 || base > 36)
    base = 10;

  s = nptr;

  /* Skip white space.  */
  while (isspace (*s))
    ++s;
  if (*s == '\0')
    goto noconv;

  /* Check for a sign.  */
  if (*s == '-')
    {
      negative = 1;
      ++s;
    }
  else if (*s == '+')
    {
      negative = 0;
      ++s;
    }
  else
    negative = 0;

  if (base == 16 && s[0] == '0' && toupper (s[1]) == 'X')
    s += 2;

  /* If BASE is zero, figure it out ourselves.  */
  if (base == 0)
    if (*s == '0')
      {
	if (toupper (s[1]) == 'X')
	  {
	    s += 2;
	    base = 16;
	  }
	else
	  base = 8;
      }
    else
      base = 10;

  /* Save the pointer so we can check later if anything happened.  */
  save = s;

#ifdef USE_NUMBER_GROUPING
  if (group)
    {
      /* Find the end of the digit string and check its grouping.  */
      end = s;
      for (c = *end; c != '\0'; c = *++end)
	if (c != thousands && !isdigit (c) &&
	    (!isalpha (c) || toupper (c) - 'A' + 10 >= base))
	  break;
      if (*s == thousands)
	end = s;
      else
	end = correctly_grouped_prefix (s, end, thousands, grouping);
    }
  else
#endif
    end = NULL;

  cutoff = ULONG_MAX / (unsigned LONG int) base;
  cutlim = ULONG_MAX % (unsigned LONG int) base;

  overflow = 0;
  i = 0;
  for (c = *s; c != '\0'; c = *++s)
    {
      if (s == end)
	break;
      if (isdigit (c))
	c -= '0';
      else if (isalpha (c))
	c = toupper (c) - 'A' + 10;
      else
	break;
      if (c >= base)
	break;
      /* Check for overflow.  */
      if (i > cutoff || (i == cutoff && c > cutlim))
	overflow = 1;
      else
	{
	  i *= (unsigned LONG int) base;
	  i += c;
	}
    }

  /* Check if anything actually happened.  */
  if (s == save)
    goto noconv;

  /* Store in ENDPTR the address of one character
     past the last character we converted.  */
  if (endptr != NULL)
    *endptr = (char *) s;

#if !UNSIGNED
  /* Check for a value that is within the range of
     `unsigned LONG int', but outside the range of `LONG int'.  */
  if (i > (negative ?
	   -(unsigned LONG int) LONG_MIN : (unsigned LONG int) LONG_MAX))
    overflow = 1;
#endif

  if (overflow)
    {
      errno = ERANGE;
#if UNSIGNED
      return ULONG_MAX;
#else
      return negative ? LONG_MIN : LONG_MAX;
#endif
    }

  /* Return the result of the appropriate sign.  */
  return (negative ? -i : i);

noconv:
  /* There was no number to convert.  */
  if (endptr != NULL)
    *endptr = (char *) nptr;
  return 0L;
}

/* External user entry point.  */

INT
strtol (nptr, endptr, base)
     const char *nptr;
     char **endptr;
     int base;
{
  return INTERNAL (strtol) (nptr, endptr, base, 0);
}

#ifdef weak_symbol
weak_symbol (strtol)
#endif
