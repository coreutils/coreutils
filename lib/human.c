/* human.c -- print human readable file size
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.

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

/* Originally contributed by lm@sgi.com;
   --si and large file support added by eggert@twinsun.com.  */

#include <config.h>

#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif

#include <sys/types.h>
#include <stdio.h>

#if HAVE_LIMITS_H
# include <limits.h>
#endif

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#include "human.h"

static const char suffixes[] = 
{
  0,	/* not used */
  'k',	/* kilo */
  'M',	/* Mega */
  'G',	/* Giga */
  'T',	/* Tera */
  'P',	/* Peta */
  'E',	/* Exa */
  'Z',	/* Zetta */
  'Y'	/* Yotta */
};

/* Convert N to a human readable format in BUF.
   
   N is expressed in units of FROM_UNITS; use units of TO_UNITS in the
   output number.  FROM_UNITS and TO_UNITS must be positive, and one must
   be a multiple of the other.
   
   If BASE is nonzero, use a format like "127k" if possible,
   using powers of BASE; otherwise, use ordinary decimal format.
   Normally BASE is either 1000 or 1024; it must be at least 2.
   Most people visually process strings of 3-4 digits effectively,
   but longer strings of digits are more prone to misinterpretation.
   Hence, converting to an abbreviated form usually improves readability.
   Use a suffix indicating which power is being used.
   For example, assuming BASE is 1024, 8500 would be converted to 8.3k,
   133456345 to 127M, 56990456345 to 53G, and so on.  Numbers smaller
   than BASE aren't modified.  */

char *
human_readable (n, buf, from_units, to_units, base)
     uintmax_t n;
     char *buf;
     int from_units;
     int to_units;
     int base;
{
  uintmax_t amt;
  int tenths;
  int power;
  char *p;

  /* 0 means adjusted N == AMT.TENTHS;
     1 means AMT.TENTHS < adjusted N < AMT.TENTHS + 0.05;
     2 means adjusted N == AMT.TENTHS + 0.05;
     3 means AMT.TENTHS + 0.05 < adjusted N < AMT.TENTHS + 0.1.  */
  int rounding;

  p = buf + LONGEST_HUMAN_READABLE;
  *p = '\0';
   

  /* Adjust AMT out of FROM_UNITS units and into TO_UNITS units.  */
   
  if (to_units <= from_units)
    {
      int multiplier = from_units / to_units;
      amt = n * multiplier;
      tenths = rounding = 0;

      if (amt / multiplier != n)
	{
	  /* Overflow occurred during multiplication.  We should use
	     multiple precision arithmetic here, but we'll be lazy and
	     resort to floating point.  This can yield answers that
	     are slightly off.  In practice it is quite rare to
	     overflow uintmax_t, so this is good enough for now.  */

	  double damt = n * (double) multiplier;

	  if (! base)
	    sprintf (buf, "%.0f", damt);
	  else
	    {
	      double e = 1;
	      power = 0;

	      do
		{
		  e *= base;
		  power++;
		}
	      while (e * base <= amt && power < sizeof suffixes - 1);

	      damt /= e;

	      sprintf (buf, "%.1f%c", damt, suffixes[power]);
	      if (4 < strlen (buf))
		sprintf (buf, "%.0f%c", damt, suffixes[power]);
	    }

	  return buf;
	}
    }
  else
    {
      int divisor = to_units / from_units;
      int r10 = (n % divisor) * 10;
      int r2 = (r10 % divisor) * 2;
      amt = n / divisor;
      tenths = r10 / divisor;
      rounding = r2 < divisor ? 0 < r2 : 2 + (divisor < r2);
    }

   
  /* Use power of BASE notation if adjusted AMT is large enough.  */

  if (base && base <= amt)
    {
      power = 0;

      do
	{
	  int r10 = (amt % base) * 10 + tenths;
	  int r2 = (r10 % base) * 2 + (rounding >> 1);
	  amt /= base;
	  tenths = r10 / base;
	  rounding = (r2 < base
		      ? 0 < r2 + rounding
		      : 2 + (base < r2 + rounding));
	  power++;
	}
      while (base <= amt && power < sizeof suffixes - 1);

      *--p = suffixes[power];

      if (amt < 10)
	{
	  tenths += 2 < rounding + (tenths & 1);

	  if (tenths == 10)
	    {
	      amt++;
	      tenths = 0;
	    }

	  if (amt < 10)
	    {
	      *--p = '0' + tenths;
	      *--p = '.';
	      tenths = 0;
	    }
	}
    }
   
  if (5 < tenths + (2 < rounding + (amt & 1)))
    {
      amt++;

      if (amt == base && power < sizeof suffixes - 1)
	{
	  *p = suffixes[power + 1];
	  *--p = '0';
	  *--p = '.';
	  amt = 1;
	}
    }

  do
    *--p = '0' + (int) (amt % 10);
  while ((amt /= 10) != 0);

  return p;
}
