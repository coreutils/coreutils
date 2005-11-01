/* Parse dates for touch and date.

   Copyright (C) 1989, 1990, 1991, 1998, 2000, 2001, 2002, 2003, 2004, 2005
   Free Software Foundation Inc.

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

/* Yacc-based version written by Jim Kingdon and David MacKenzie.
   Rewritten by Jim Meyering.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#ifdef TM_IN_SYS_TIME
# include <sys/time.h>
#else
# include <time.h>
#endif

#include "posixtm.h"

#if USE_UNLOCKED_IO
# include "unlocked-io.h"
#endif

/* ISDIGIT differs from isdigit, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char.
   - It's guaranteed to evaluate its argument exactly once.
   - It's typically faster.
   POSIX says that only '0' through '9' are digits.  Prefer ISDIGIT to
   ISDIGIT_LOCALE unless it's important to use the locale's definition
   of `digit' even when the host does not conform to POSIX.  */
#define ISDIGIT(c) ((unsigned int) (c) - '0' <= 9)

time_t mktime ();

/*
  POSIX requires:

  touch -t [[CC]YY]mmddhhmm[.ss] FILE...
    8, 10, or 12 digits, followed by optional .ss
    (PDS_LEADING_YEAR | PDS_CENTURY | PDS_SECONDS)

  touch mmddhhmm[YY] FILE... (obsoleted by POSIX 1003.1-2001)
    8 or 10 digits, YY (if present) must be in the range 69-99
    (PDS_TRAILING_YEAR | PDS_PRE_2000)

  date mmddhhmm[[CC]YY]
    8, 10, or 12 digits
    (PDS_TRAILING_YEAR | PDS_CENTURY)

*/

static int
year (struct tm *tm, const int *digit_pair, size_t n, unsigned int syntax_bits)
{
  switch (n)
    {
    case 1:
      tm->tm_year = *digit_pair;
      /* Deduce the century based on the year.
	 POSIX requires that 00-68 be interpreted as 2000-2068,
	 and that 69-99 be interpreted as 1969-1999.  */
      if (digit_pair[0] <= 68)
	{
	  if (syntax_bits & PDS_PRE_2000)
	    return 1;
	  tm->tm_year += 100;
	}
      break;

    case 2:
      if (! (syntax_bits & PDS_CENTURY))
	return 1;
      tm->tm_year = digit_pair[0] * 100 + digit_pair[1] - 1900;
      break;

    case 0:
      {
	time_t now;
	struct tm *tmp;

	/* Use current year.  */
	time (&now);
	tmp = localtime (&now);
	if (! tmp)
	  return 1;
	tm->tm_year = tmp->tm_year;
      }
      break;

    default:
      abort ();
    }

  return 0;
}

static int
posix_time_parse (struct tm *tm, const char *s, unsigned int syntax_bits)
{
  const char *dot = NULL;
  int pair[6];
  int *p;
  size_t i;

  size_t s_len = strlen (s);
  size_t len = (((syntax_bits & PDS_SECONDS) && (dot = strchr (s, '.')))
		? (size_t) (dot - s)
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
      if (year (tm, p, len - 4, syntax_bits))
	return 1;
      p += len - 4;
      len = 4;
    }

  /* Handle 8 digits worth of `MMDDhhmm'.  */
  tm->tm_mon = *p++ - 1;
  tm->tm_mday = *p++;
  tm->tm_hour = *p++;
  tm->tm_min = *p++;
  len -= 4;

  /* Handle any trailing year.  */
  if (syntax_bits & PDS_TRAILING_YEAR)
    {
      if (year (tm, p, len, syntax_bits))
	return 1;
    }

  /* Handle seconds.  */
  if (!dot)
    {
      tm->tm_sec = 0;
    }
  else
    {
      int seconds;

      ++dot;
      if (!ISDIGIT (dot[0]) || !ISDIGIT (dot[1]))
	return 1;
      seconds = 10 * (dot[0] - '0') + dot[1] - '0';

      tm->tm_sec = seconds;
    }

  return 0;
}

/* Parse a POSIX-style date, returning true if successful.  */

bool
posixtime (time_t *p, const char *s, unsigned int syntax_bits)
{
  struct tm tm0
#ifdef lint
  /* Placate gcc-4's -Wuninitialized.
     posix_time_parse fails to set all of tm0 only when it returns
     nonzero (due to year() returning nonzero), and in that case,
     this code doesn't use the tm0 at all.  */
    = { 0, }
#endif
    ;
  struct tm tm1;
  struct tm const *tm;
  time_t t;

  if (posix_time_parse (&tm0, s, syntax_bits))
    return false;

  tm1 = tm0;
  tm1.tm_isdst = -1;
  t = mktime (&tm1);

  if (t != (time_t) -1)
    tm = &tm1;
  else
    {
      /* mktime returns -1 for errors, but -1 is also a valid time_t
	 value.  Check whether an error really occurred.  */
      tm = localtime (&t);
      if (! tm)
	return false;
    }

  /* Reject dates like "September 31" and times like "25:61".  */
  if ((tm0.tm_year ^ tm->tm_year)
      | (tm0.tm_mon ^ tm->tm_mon)
      | (tm0.tm_mday ^ tm->tm_mday)
      | (tm0.tm_hour ^ tm->tm_hour)
      | (tm0.tm_min ^ tm->tm_min)
      | (tm0.tm_sec ^ tm->tm_sec))
    return false;

  *p = t;
  return true;
}

#ifdef TEST_POSIXTIME
/*
    Test mainly with syntax_bits == 13
    (aka: (PDS_LEADING_YEAR | PDS_CENTURY | PDS_SECONDS))

    This test data assumes Universal Time, e.g., TZ="UTC0".

    This test data also assumes that time_t is signed and is at least
    39 bits wide, so that it can represent all years from 0000 through
    9999.  A host with 32-bit signed time_t can represent only time
    stamps in the range 1901-12-13 20:45:52 through 2038-01-18
    03:14:07 UTC, assuming POSIX time_t with no leap seconds, so test
    cases outside this range will not work on such a host.

    Also, the first two lines of test data assume that the current
    year is 2002.

BEGIN-DATA
12131415.16     13   1039788916 Fri Dec 13 14:15:16 2002
12131415.16     13   1039788916 Fri Dec 13 14:15:16 2002
000001010000.00 13 -62167132800 Sun Jan  1 00:00:00 0000
190112132045.52 13  -2147483648 Fri Dec 13 20:45:52 1901
190112132045.53 13  -2147483647 Fri Dec 13 20:45:53 1901
190112132046.52 13  -2147483588 Fri Dec 13 20:46:52 1901
190112132145.52 13  -2147480048 Fri Dec 13 21:45:52 1901
190112142045.52 13  -2147397248 Sat Dec 14 20:45:52 1901
190201132045.52 13  -2144805248 Mon Jan 13 20:45:52 1902
196912312359.59 13           -1 Wed Dec 31 23:59:59 1969
197001010000.00 13            0 Thu Jan  1 00:00:00 1970
197001010000.01 13            1 Thu Jan  1 00:00:01 1970
197001010001.00 13           60 Thu Jan  1 00:01:00 1970
197001010100.00 13         3600 Thu Jan  1 01:00:00 1970
197001020000.00 13        86400 Fri Jan  2 00:00:00 1970
197002010000.00 13      2678400 Sun Feb  1 00:00:00 1970
197101010000.00 13     31536000 Fri Jan  1 00:00:00 1971
197001000000.00 13            * *
197000010000.00 13            * *
197001010000.60 13            * *
197001010060.00 13            * *
197001012400.00 13            * *
197001320000.00 13            * *
197013010000.00 13            * *
203801190314.06 13   2147483646 Tue Jan 19 03:14:06 2038
203801190314.07 13   2147483647 Tue Jan 19 03:14:07 2038
203801190314.08 13   2147483648 Tue Jan 19 03:14:08 2038
999912312359.59 13 253402300799 Fri Dec 31 23:59:59 9999
1112131415      13   1323785700 Tue Dec 13 14:15:00 2011
1112131415.16   13   1323785716 Tue Dec 13 14:15:16 2011
201112131415.16 13   1323785716 Tue Dec 13 14:15:16 2011
191112131415.16 13  -1831974284 Wed Dec 13 14:15:16 1911
203712131415.16 13   2144326516 Sun Dec 13 14:15:16 2037
3712131415.16   13   2144326516 Sun Dec 13 14:15:16 2037
6812131415.16   13   3122633716 Thu Dec 13 14:15:16 2068
6912131415.16   13     -1590284 Sat Dec 13 14:15:16 1969
7012131415.16   13     29945716 Sun Dec 13 14:15:16 1970
1213141599       2    945094500 Mon Dec 13 14:15:00 1999
1213141500       2    976716900 Wed Dec 13 14:15:00 2000
END-DATA

*/

# define MAX_BUFF_LEN 1024

int
main (void)
{
  char buff[MAX_BUFF_LEN + 1];

  buff[MAX_BUFF_LEN] = 0;
  while (fgets (buff, MAX_BUFF_LEN, stdin) && buff[0])
    {
      char time_str[MAX_BUFF_LEN];
      unsigned int syntax_bits;
      time_t t;
      if (sscanf (buff, "%s %u", time_str, &syntax_bits) != 2)
	printf ("*\n");
      else
	{
	  printf ("%-15s %2u ", time_str, syntax_bits);
	  if (posixtime (&t, time_str, syntax_bits))
	    printf ("%12ld %s", (long int) t, ctime (&t));
	  else
	    printf ("%12s %s", "*", "*\n");
	}
    }
  exit (0);

}
#endif

/*
Local Variables:
compile-command: "gcc -DTEST_POSIXTIME -DHAVE_CONFIG_H -I.. -g -O -Wall -W posixtm.c"
End:
*/
