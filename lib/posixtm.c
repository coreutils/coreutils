/* Parse dates for touch.
   Copyright (C) 1989, 1990, 1991, 1998 Free Software Foundation Inc.

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

/* Yacc-based version written by Jim Kingdon and David MacKenzie.
   Rewritten by Jim Meyering.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>

#ifdef TM_IN_SYS_TIME
# include <sys/time.h>
#else
# include <time.h>
#endif

/* The return value. */
static struct tm t;

time_t mktime ();

/*
  POSIX requires:

  touch -t [[CC]YY]mmddhhmm[.ss] FILE...
    8, 10, or 12 digits, followed by optional .ss
    (PDS_LEADING_YEAR | PDS_CENTURY | PDS_SECONDS)

  touch mmddhhmm[YY] FILE...
    8 or 10 digits
    (PDS_TRAILING_YEAR)

  date mmddhhmm[[CC]YY]
    8, 10, or 12 digits
    (PDS_TRAILING_YEAR | PDS_CENTURY)

*/

/* FIXME: put these in posixtm.h */
/* POSIX Date Syntax flags.  */
#define PDS_LEADING_YEAR 1
#define PDS_TRAILING_YEAR 2
#define PDS_CENTURY 4
#define PDS_SECONDS 8

static int
year (const int *digit_pair, size_t n, int allow_century)
{
  switch (n)
    {
    case 1:
      t.tm_year = *digit_pair;
      /* Deduce the century based on the year.
	 See POSIX.2 section 4.63.3.  */
      if (digit_pair[0] <= 68)
	t.tm_year += 100;
      break;

    case 2:
      if (!allow_century)
	return 1;
      t.tm_year = digit_pair[0] * 100 + digit_pair[1];
      if (t.tm_year < 1900)
	return 1;
      t.tm_year -= 1900;
      break;

    case 0:
      {
	time_t now;
	struct tm *tmp;

	/* Use current year.  */
	time (&now);
	tmp = localtime (&now);
	t.tm_year = tmp->tm_year;
      }
      break;

    default:
      abort ();
    }

  return 0;
}

static int
posix_time_parse (const char *s, unsigned int syntax_bits)
{
  const char *dot;
  int pair[6];
  int *p;
  int i;

  size_t s_len = strlen (s);
  size_t len = (((syntax_bits & PDS_SECONDS) && (dot = strchr (s, '.')))
		? dot - s
		: s_len);

  if (len != 8 && len != 10 && len != 12)
    return 1;

  if (dot)
    {
      if (!(syntax_bits & PDS_SECONDS))
	return 1;

      if (s_len - len != 3)
	return 1;
    }

  for (i = 0; i < len; i++)
    if (!ISDIGIT (s[i]))
      return 1;

  len /= 2;
  for (i = 0; i < len; i++)
    pair[i] = 10 * (s[2*i] - '0') + s[2*i + 1] - '0';

  p = pair;
  if (syntax_bits & PDS_LEADING_YEAR)
    {
      if (year (p, len - 4, syntax_bits & PDS_CENTURY))
	return 1;
      p += len - 4;
      len = 4;
    }

  /* Handle 8 digits worth of `MMDDhhmm'.  */
  if (*p < 1 || *p > 12)
    return 1;
  t.tm_mon = *p - 1;

  ++p; --len;
  if (*p < 1 || *p > 31)
    return 1;
  t.tm_mday = *p;

  ++p; --len;
  if (*p < 0 || *p > 23)
    return 1;
  t.tm_hour = *p;

  ++p; --len;
  if (*p < 0 || *p > 59)
    return 1;
  t.tm_min = *p;

  /* Handle any trailing year.  */
  if (syntax_bits & PDS_TRAILING_YEAR)
    {
      if (year (p, len, syntax_bits & PDS_CENTURY))
	return 1;
    }

  /* Handle seconds.  */
  if (!dot)
    {
      t.tm_sec = 0;
    }
  else
    {
      int seconds;

      ++dot;
      if (!ISDIGIT (dot[0]) || !ISDIGIT (dot[1]))
	return 1;
      seconds = 10 * (dot[0] - '0') + dot[1] - '0';

      if (seconds < 0 || seconds > 61)
	return 1;
      t.tm_sec = seconds;
    }

  return 0;
}

/* Parse a POSIX-style date and return it, or (time_t)-1 for an error.  */

struct tm *
posixtime (const char *s)
{
  t.tm_isdst = -1;
  if (posix_time_parse (s))
    return (time_t)-1;
  else
    return mktime (&t);
}

/* Parse a POSIX-style date and return it, or NULL for an error.  */

struct tm *
posixtm (const char *s)
{
  if (posixtime (s) == -1)
    return NULL;
  return &t;
}
