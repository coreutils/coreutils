/* Copyright (C) 1991, 92, 94, 95, 96 Free Software Foundation, Inc.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.  */

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
# define INT unsigned LONG int
#endif

/* Determine the name.  */
#if UNSIGNED
# ifdef USE_WIDE_CHAR
#  ifdef QUAD
#   define strtol wcstouq
#  else
#   define strtol wcstoul
#  endif
# else
#  ifdef QUAD
#   define strtol strtouq
#  else
#   define strtol strtoul
#  endif
# endif
#else
# ifdef USE_WIDE_CHAR
#  ifdef QUAD
#   define strtol wcstoq
#  else
#   define strtol wcstol
#  endif
# else
#  ifdef QUAD
#   define strtol strtoq
#  endif
# endif
#endif

/* If QUAD is defined, we are defining `strtoq' or `strtouq',
   operating on `long long int's.  */
#ifdef QUAD
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

#ifndef ULONG_MAX
# define ULONG_MAX ((unsigned long) ~(unsigned long) 0)
#endif
#ifndef LONG_MAX
# define LONG_MAX ((long int) (ULONG_MAX >> 1))
#endif
#endif

#ifdef USE_WIDE_CHAR
# include <wchar.h>
# include <wctype.h>
# define L_(ch) L##ch
# define UCHAR_TYPE wint_t
# define STRING_TYPE wchar_t
# define ISSPACE(ch) iswspace (ch)
# define ISALPHA(ch) iswalpha (ch)
# define TOUPPER(ch) towupper (ch)
#else
# define L_(ch) ch
# define UCHAR_TYPE unsigned char
# define STRING_TYPE char
# define ISSPACE(ch) isspace (ch)
# define ISALPHA(ch) isalpha (ch)
# define TOUPPER(ch) toupper (ch)
#endif

#ifdef __STDC__
# define INTERNAL(x) INTERNAL1(x)
# define INTERNAL1(x) __##x##_internal
# define WEAKNAME(x) WEAKNAME1(x)
# define WEAKNAME1(x) __##x
#else
# define INTERNAL(x) __/**/x/**/_internal
# define WEAKNAME(x) __/**/x
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
     const STRING_TYPE *nptr;
     STRING_TYPE **endptr;
     int base;
     int group;
{
  int negative;
  register unsigned LONG int cutoff;
  register unsigned int cutlim;
  register unsigned LONG int i;
  register const STRING_TYPE *s;
  register UCHAR_TYPE c;
  const STRING_TYPE *save, *end;
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

  save = s = nptr;

  /* Skip white space.  */
  while (ISSPACE (*s))
    ++s;
  if (*s == L_('\0'))
    goto noconv;

  /* Check for a sign.  */
  if (*s == L_('-'))
    {
      negative = 1;
      ++s;
    }
  else if (*s == L_('+'))
    {
      negative = 0;
      ++s;
    }
  else
    negative = 0;

  if (base == 16 && s[0] == L_('0') && TOUPPER (s[1]) == L_('X'))
    s += 2;

  /* If BASE is zero, figure it out ourselves.  */
  if (base == 0)
    if (*s == L_('0'))
      {
	if (TOUPPER (s[1]) == L_('X'))
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
      for (c = *end; c != L_('\0'); c = *++end)
	if (c != thousands && (c < L_('0') || c > L_('9'))
	    && (!ISALPHA (c) || TOUPPER (c) - L_('A') + 10 >= base))
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
  for (c = *s; c != L_('\0'); c = *++s)
    {
      if (s == end)
	break;
      if (c >= L_('0') && c <= L_('9'))
	c -= L_('0');
      else if (ISALPHA (c))
	c = TOUPPER (c) - L_('A') + 10;
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
    *endptr = (STRING_TYPE *) s;

#if !UNSIGNED
  /* Check for a value that is within the range of
     `unsigned LONG int', but outside the range of `LONG int'.  */
  if (overflow == 0
      && i > (negative
	      ? -((unsigned LONG int) (LONG_MIN + 1)) + 1
	      : (unsigned LONG int) LONG_MAX))
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
  /* We must handle a special case here: the base is 0 or 16 and the
     first two characters and '0' and 'x', but the rest are no
     hexadecimal digits.  This is no error case.  We return 0 and
     ENDPTR points to the `x`.  */
  if (endptr != NULL)
    if (save - nptr >= 2 && TOUPPER (save[-1]) == L_('X')
	&& save[-2] == L_('0'))
      *endptr = (STRING_TYPE *) &save[-1];
    else
      /*  There was no number to convert.  */
      *endptr = (STRING_TYPE *) nptr;

  return 0L;
}

/* External user entry point.  */

#undef __P
#if defined (__STDC__) && __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif

/* Prototype.  */
INT strtol __P ((const STRING_TYPE *nptr, STRING_TYPE **endptr,
			    int base));


INT
strtol (nptr, endptr, base)
     const STRING_TYPE *nptr;
     STRING_TYPE **endptr;
     int base;
{
  return INTERNAL (strtol) (nptr, endptr, base, 0);
}
