/* Compare numeric strings.  This is an internal include file.

   Copyright (C) 1988-2025 Free Software Foundation, Inc.

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

/* Written by Mike Haertel.  */

#ifndef STRNUMCMP_IN_H
# define STRNUMCMP_IN_H 1

# include "strnumcmp.h"

# include "c-ctype.h"
# include <stddef.h>

# define NEGATION_SIGN   '-'
# define NUMERIC_ZERO    '0'

/* Compare strings A and B containing decimal fractions < 1.
   DECIMAL_POINT is the decimal point.  Each string
   should begin with a decimal point followed immediately by the digits
   of the fraction.  Strings not of this form are treated as zero.  */

/* The goal here, is to take two numbers a and b... compare these
   in parallel.  Instead of converting each, and then comparing the
   outcome.  Most likely stopping the comparison before the conversion
   is complete.  The algorithm used, in the old "sort" utility:

   Algorithm: fraccompare
   Action   : compare two decimal fractions
   accepts  : char *a, char *b
   returns  : -1 if a<b, 0 if a=b, 1 if a>b.
   implement:

   if *a == decimal_point AND *b == decimal_point
     find first character different in a and b.
     if both are digits, return the difference *a - *b.
     if *a is a digit
       skip past zeros
       if digit return 1, else 0
     if *b is a digit
       skip past zeros
       if digit return -1, else 0
   if *a is a decimal_point
     skip past decimal_point and zeros
     if digit return 1, else 0
   if *b is a decimal_point
     skip past decimal_point and zeros
     if digit return -1, else 0
   return 0 */

static inline int _GL_ATTRIBUTE_PURE
fraccompare (char const *a, char const *b, char decimal_point)
{
  if (*a == decimal_point && *b == decimal_point)
    {
      while (*++a == *++b)
        if (! c_isdigit (*a))
          return 0;
      if (c_isdigit (*a) && c_isdigit (*b))
        return *a - *b;
      if (c_isdigit (*a))
        goto a_trailing_nonzero;
      if (c_isdigit (*b))
        goto b_trailing_nonzero;
      return 0;
    }
  else if (*a++ == decimal_point)
    {
    a_trailing_nonzero:
      while (*a == NUMERIC_ZERO)
        a++;
      return c_isdigit (*a);
    }
  else if (*b++ == decimal_point)
    {
    b_trailing_nonzero:
      while (*b == NUMERIC_ZERO)
        b++;
      return - c_isdigit (*b);
    }
  return 0;
}

/* Compare strings A and B as numbers without explicitly converting
   them to machine numbers, to avoid overflow problems and perhaps
   improve performance.  DECIMAL_POINT is the decimal point and
   THOUSANDS_SEP the thousands separator.  A DECIMAL_POINT outside
   'char' range causes comparisons to act as if there is no decimal point
   character, and likewise for THOUSANDS_SEP.  */

static inline int _GL_ATTRIBUTE_PURE
numcompare (char const *a, char const *b,
            int decimal_point, int thousands_sep)
{
  char tmpa = *a;
  char tmpb = *b;
  int tmp;
  size_t log_a;
  size_t log_b;

  if (tmpa == NEGATION_SIGN)
    {
      do
        tmpa = *++a;
      while (tmpa == NUMERIC_ZERO || tmpa == thousands_sep);
      if (tmpb != NEGATION_SIGN)
        {
          if (tmpa == decimal_point)
            do
              tmpa = *++a;
            while (tmpa == NUMERIC_ZERO);
          if (c_isdigit (tmpa))
            return -1;
          while (tmpb == NUMERIC_ZERO || tmpb == thousands_sep)
            tmpb = *++b;
          if (tmpb == decimal_point)
            do
              tmpb = *++b;
            while (tmpb == NUMERIC_ZERO);
          return - c_isdigit (tmpb);
        }
      do
        tmpb = *++b;
      while (tmpb == NUMERIC_ZERO || tmpb == thousands_sep);

      while (tmpa == tmpb && c_isdigit (tmpa))
        {
          do
            tmpa = *++a;
          while (tmpa == thousands_sep);
          do
            tmpb = *++b;
          while (tmpb == thousands_sep);
        }

      if ((tmpa == decimal_point && !c_isdigit (tmpb))
          || (tmpb == decimal_point && !c_isdigit (tmpa)))
        return fraccompare (b, a, decimal_point);

      tmp = tmpb - tmpa;

      for (log_a = 0; c_isdigit (tmpa); ++log_a)
        do
          tmpa = *++a;
        while (tmpa == thousands_sep);

      for (log_b = 0; c_isdigit (tmpb); ++log_b)
        do
          tmpb = *++b;
        while (tmpb == thousands_sep);

      if (log_a != log_b)
        return log_a < log_b ? 1 : -1;

      if (!log_a)
        return 0;

      return tmp;
    }
  else if (tmpb == NEGATION_SIGN)
    {
      do
        tmpb = *++b;
      while (tmpb == NUMERIC_ZERO || tmpb == thousands_sep);
      if (tmpb == decimal_point)
        do
          tmpb = *++b;
        while (tmpb == NUMERIC_ZERO);
      if (c_isdigit (tmpb))
        return 1;
      while (tmpa == NUMERIC_ZERO || tmpa == thousands_sep)
        tmpa = *++a;
      if (tmpa == decimal_point)
        do
          tmpa = *++a;
        while (tmpa == NUMERIC_ZERO);
      return c_isdigit (tmpa);
    }
  else
    {
      while (tmpa == NUMERIC_ZERO || tmpa == thousands_sep)
        tmpa = *++a;
      while (tmpb == NUMERIC_ZERO || tmpb == thousands_sep)
        tmpb = *++b;

      while (tmpa == tmpb && c_isdigit (tmpa))
        {
          do
            tmpa = *++a;
          while (tmpa == thousands_sep);
          do
            tmpb = *++b;
          while (tmpb == thousands_sep);
        }

      if ((tmpa == decimal_point && !c_isdigit (tmpb))
          || (tmpb == decimal_point && !c_isdigit (tmpa)))
        return fraccompare (a, b, decimal_point);

      tmp = tmpa - tmpb;

      for (log_a = 0; c_isdigit (tmpa); ++log_a)
        do
          tmpa = *++a;
        while (tmpa == thousands_sep);

      for (log_b = 0; c_isdigit (tmpb); ++log_b)
        do
          tmpb = *++b;
        while (tmpb == thousands_sep);

      if (log_a != log_b)
        return log_a < log_b ? -1 : 1;

      if (!log_a)
        return 0;

      return tmp;
    }
}

#endif
