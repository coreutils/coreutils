/* Copyright (C) 1991, 92, 93, 94, 95, 96 Free Software Foundation, Inc.

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
# define HAVE_LIMITS_H 1
# define HAVE_MBLEN 1
# define HAVE_TM_ZONE 1
# define STDC_HEADERS 1
# include <ansidecl.h>
# include "../locale/localeinfo.h"
#endif

#include <stdio.h>
#include <sys/types.h>		/* Some systems define `time_t' here.  */

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_MBLEN
# include <ctype.h>
#endif

#if HAVE_LIMITS_H
# include <limits.h>
#endif

#if STDC_HEADERS
# include <stddef.h>
# include <stdlib.h>
# include <string.h>
#else
# define memcpy(d, s, n) bcopy (s, d, n)
#endif

#ifndef __P
#if defined (__GNUC__) || (defined (__STDC__) && __STDC__)
#define __P(args) args
#else
#define __P(args) ()
#endif  /* GCC.  */
#endif  /* Not __P.  */

#ifndef PTR
#ifdef __STDC__
#define PTR void *
#else
#define PTR char *
#endif
#endif

/* Uncomment following line in the production version.  */
/* #define NDEBUG */
#include <assert.h>

static unsigned int week __P ((const struct tm *const, int, int));


#define	add(n, f)							      \
  do									      \
    {									      \
      i += (n);								      \
      if (i >= maxsize)							      \
	return 0;							      \
      else								      \
	if (p)								      \
	  {								      \
	    f;								      \
	    p += (n);							      \
	  }								      \
    } while (0)
#define	cpy(n, s)	add ((n), memcpy((PTR) p, (PTR) (s), (n)))

#ifdef _LIBC
#define	fmt(n, args)	add((n), if (sprintf args != (n)) return 0)
#else
#define	fmt(n, args)	add((n), sprintf args; if (strlen (p) != (n)) return 0)
#endif



/* Return the week in the year specified by TP,
   with weeks starting on STARTING_DAY.  */
#ifdef	__GNUC__
inline
#endif
static unsigned int
week (tp, starting_day, max_preceding)
      const struct tm *const tp;
      int starting_day;
      int max_preceding;
{
  int wday, dl, base;

  wday = tp->tm_wday - starting_day;
  if (wday < 0)
    wday += 7;

  /* Set DL to the day in the year of the first day of the week
     containing the day specified in TP.  */
  dl = tp->tm_yday - wday;

  /* For the computation following ISO 8601:1988 we set the number of
     the week containing January 1st to 1 if this week has more than
     MAX_PRECEDING days in the new year.  For ISO 8601 this number is
     3, for the other representation it is 7 (i.e., not to be
     fulfilled).  */
  base = ((dl + 7) % 7) > max_preceding ? 1 : 0;

  /* If DL is negative we compute the result as 0 unless we have to
     compute it according ISO 8601.  In this case we have to return 53
     or 1 if the week containing January 1st has less than 4 days in
     the new year or not.  If DL is not negative we calculate the
     number of complete weeks for our week (DL / 7) plus 1 (because
     only for DL < 0 we are in week 0/53 and plus the number of the
     first week computed in the last step.  */
  return dl < 0 ? (dl < -max_preceding ? 53 : base)
		: base + 1 + dl / 7;
}

#ifndef _NL_CURRENT
static char const weekday_name[][10] =
  {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
  };
static char const month_name[][10] =
  {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
  };
#endif

/* Write information from TP into S according to the format
   string FORMAT, writing no more that MAXSIZE characters
   (including the terminating '\0') and returning number of
   characters written.  If S is NULL, nothing will be written
   anywhere, so to determine how many characters would be
   written, use NULL for S and (size_t) UINT_MAX for MAXSIZE.  */
size_t
strftime (s, maxsize, format, tp)
      char *s;
      size_t maxsize;
      const char *format;
      register const struct tm *tp;
{
  int hour12 = tp->tm_hour;
#ifdef _NL_CURRENT
  const char *const a_wkday = _NL_CURRENT (LC_TIME, ABDAY_1 + tp->tm_wday);
  const char *const f_wkday = _NL_CURRENT (LC_TIME, DAY_1 + tp->tm_wday);
  const char *const a_month = _NL_CURRENT (LC_TIME, ABMON_1 + tp->tm_mon);
  const char *const f_month = _NL_CURRENT (LC_TIME, MON_1 + tp->tm_mon);
  const char *const ampm = _NL_CURRENT (LC_TIME,
					hour12 > 11 ? PM_STR : AM_STR);
  size_t aw_len = strlen(a_wkday);
  size_t am_len = strlen(a_month);
  size_t ap_len = strlen (ampm);
#else
  const char *const f_wkday = weekday_name[tp->tm_wday];
  const char *const f_month = month_name[tp->tm_mon];
  const char *const a_wkday = f_wkday;
  const char *const a_month = f_month;
  const char *const ampm = "AMPM" + 2 * (hour12 > 11);
  size_t aw_len = 3;
  size_t am_len = 3;
  size_t ap_len = 2;
#endif
  size_t wkday_len = strlen (f_wkday);
  size_t month_len = strlen (f_month);
  const unsigned int y_week0 = week (tp, 0, 7);
  const unsigned int y_week1 = week (tp, 1, 7);
  const unsigned int y_week2 = week (tp, 1, 3);
  const char *zone;
  size_t zonelen;
  register size_t i = 0;
  register char *p = s;
  register const char *f;
  char number_fmt[5];

  /* Initialize the buffer we will use for the sprintf format for numbers.  */
  number_fmt[0] = '%';

  zone = 0;
#if HAVE_TM_ZONE
  zone = (const char *) tp->tm_zone;
#endif
#if HAVE_TZNAME
  if (!(zone && *zone) && tp->tm_isdst >= 0)
    zone = tzname[tp->tm_isdst];
#endif
  if (!(zone && *zone))
    zone = "???";

  zonelen = strlen (zone);

  if (hour12 > 12)
    hour12 -= 12;
  else
    if (hour12 == 0) hour12 = 12;

  for (f = format; *f != '\0'; ++f)
    {
      enum { pad_zero, pad_space, pad_none } pad; /* Padding for number.  */
      unsigned int maxdigits;	/* Max digits for numeric format.  */
      unsigned int number_value; /* Numeric value to be printed.  */
      const char *subfmt;

#if HAVE_MBLEN
      if (!isascii (*f))
	{
	  /* Non-ASCII, may be a multibyte.  */
	  int len = mblen (f, strlen (f));
	  if (len > 0)
	    {
	      cpy(len, f);
	      continue;
	    }
	}
#endif

      if (*f != '%')
	{
	  add (1, *p = *f);
	  continue;
	}

      /* Check for flags that can modify a number format.  */
      ++f;
      switch (*f)
	{
	case '_':
	  pad = pad_space;
	  ++f;
	  break;
	case '-':
	  pad = pad_none;
	  ++f;
	  break;
	default:
	  pad = pad_zero;
	  break;
	}

      /* Now do the specified format.  */
      switch (*f)
	{
	case '\0':
	case '%':
	  add (1, *p = *f);
	  break;

	case 'a':
	  cpy (aw_len, a_wkday);
	  break;

	case 'A':
	  cpy (wkday_len, f_wkday);
	  break;

	case 'b':
	case 'h':		/* GNU extension.  */
	  cpy (am_len, a_month);
	  break;

	case 'B':
	  cpy (month_len, f_month);
	  break;

	case 'c':
#ifdef _NL_CURRENT
	  subfmt = _NL_CURRENT (LC_TIME, D_T_FMT);
#else
	  subfmt = "%a %b %d %H:%M:%S %Z %Y";
#endif
	subformat:
	  {
	    size_t len = strftime (p, maxsize - i, subfmt, tp);
	    if (len == 0 && *subfmt)
	      return 0;
	    add (len, ;);
	  }
	  break;

#define DO_NUMBER(digits, value) \
	  maxdigits = digits; number_value = value; goto do_number
#define DO_NUMBER_SPACEPAD(digits, value) \
	  maxdigits = digits; number_value = value; goto do_number_spacepad

	case 'C':
	  DO_NUMBER (2, (1900 + tp->tm_year) / 100);

	case 'x':
#ifdef _NL_CURRENT
	  subfmt = _NL_CURRENT (LC_TIME, D_FMT);
	  goto subformat;
#endif
	  /* Fall through.  */
	case 'D':		/* GNU extension.  */
	  subfmt = "%m/%d/%y";
	  goto subformat;

	case 'd':
	  DO_NUMBER (2, tp->tm_mday);

	case 'e':		/* GNU extension: %d, but blank-padded.  */
	  DO_NUMBER_SPACEPAD (2, tp->tm_mday);

	  /* All numeric formats set MAXDIGITS and NUMBER_VALUE and then
	     jump to one of these two labels.  */

	do_number_spacepad:
	  /* Force `_' flag.  */
	  pad = pad_space;

	do_number:
	  {
	    /* Format the number according to the PAD flag.  */

	    register char *nf = &number_fmt[1];
	    int printed = maxdigits;

	    switch (pad)
	      {
	      case pad_zero:
		*nf++ = '0';
	      case pad_space:
		*nf++ = '0' + maxdigits;
	      case pad_none:
		*nf++ = 'u';
		*nf = '\0';
	      }

#ifdef _LIBC
	    add (maxdigits, printed = sprintf (p, number_fmt, number_value));
#else
	    add (maxdigits, sprintf (p, number_fmt, number_value);
		 printed = strlen (p));
#endif
	    /* Back up if fewer than MAXDIGITS chars written for pad_none.  */
	    p -= maxdigits - printed;
	    i -= maxdigits - printed;

	    break;
	  }


	case 'H':
	  DO_NUMBER (2, tp->tm_hour);

	case 'I':
	  DO_NUMBER (2, hour12);

	case 'k':		/* GNU extension.  */
	  DO_NUMBER_SPACEPAD (2, tp->tm_hour);

	case 'l':		/* GNU extension.  */
	  DO_NUMBER_SPACEPAD (2, hour12);

	case 'j':
	  DO_NUMBER (3, 1 + tp->tm_yday);

	case 'M':
	  DO_NUMBER (2, tp->tm_min);

	case 'm':
	  DO_NUMBER (2, tp->tm_mon + 1);

	case 'n':		/* GNU extension.  */
	  add (1, *p = '\n');
	  break;

	case 'p':
	  cpy (ap_len, ampm);
	  break;

	case 'R':		/* GNU extension.  */
	  subfmt = "%H:%M";
	  goto subformat;

	case 'r':		/* GNU extension.  */
	  subfmt = "%I:%M:%S %p";
	  goto subformat;

	case 'S':
	  DO_NUMBER (2, tp->tm_sec);

	case 's':		/* GNU extension.  */
	  {
	    struct tm writable_tm = *tp;
	    unsigned long int num = (unsigned long int) mktime (&writable_tm);
	    /* `3 * sizeof (unsigned long int)' is an approximation of
	       the size of the decimal representation of NUM, valid
	       for sizes <= 16.  */
	    int printed = 3 * sizeof (unsigned long int);
	    maxdigits = printed;
	    assert (sizeof (unsigned long int) <= 16);
#ifdef _LIBC
	    add (maxdigits, printed = sprintf (p, "%lu", num));
#else
	    add (maxdigits, sprintf (p, "%lu", num); printed = strlen (p));
#endif
	    /* Back up if fewer than MAXDIGITS chars written for pad_none.  */
	    p -= maxdigits - printed;
	    i -= maxdigits - printed;
	  }
	break;

	case 'X':
#ifdef _NL_CURRENT
	  subfmt = _NL_CURRENT (LC_TIME, T_FMT);
	  goto subformat;
#endif
	  /* Fall through.  */
	case 'T':		/* GNU extension.  */
	  subfmt = "%H:%M:%S";
	  goto subformat;

	case 't':		/* GNU extension.  */
	  add (1, *p = '\t');
	  break;

	case 'U':
	  DO_NUMBER (2, y_week0);

	case 'V':
	  DO_NUMBER (2, y_week2);

	case 'W':
	  DO_NUMBER (2, y_week1);

	case 'w':
	  DO_NUMBER (2, tp->tm_wday);

	case 'Y':
	  DO_NUMBER (4, 1900 + tp->tm_year);

	case 'y':
	  DO_NUMBER (2, tp->tm_year % 100);

	case 'Z':
	  cpy(zonelen, zone);
	  break;

	case 'z':
	  {
	    struct tm tml = *tp;
	    struct tm tmg;
	    time_t t;
	    time_t offset = 0;
	    int diff;

	    t = __mktime_internal (&tml, __localtime_r, &offset);

	    /* Canonicalize the local time.  */
	    if (t == (time_t) -1 || __localtime_r (&t, &tml) == NULL)
	      /* We didn't managed to get the local time.  Assume it
		 GMT as a reasonable default value.  */
	      diff = 0;
	    else
	      {
		__gmtime_r (&t, &tmg);

		/* Compute the difference.  */
		diff = tml.tm_min - tmg.tm_min;
		diff += 60 * (tml.tm_hour - tmg.tm_hour);

		if (tml.tm_mon != tmg.tm_mon)
		  {
		    /* We assume no timezone differs from UTC by more
		       than +- 23 hours.  This should be safe.  */
		    if (tmg.tm_mday == 1)
		      tml.tm_mday = 0;
		    else /* tml.tm_mday == 1 */
		      tmg.tm_mday = 0;
		  }
		diff += 1440 * (tml.tm_mday - tmg.tm_mday);
	      }

	    if (diff < 0)
	      {
		add (1, *p = '-');
		diff = -diff;
	      }
	    else
	      add (1, *p = '+');

	    pad = pad_zero;
	    DO_NUMBER (4, ((diff / 60) % 24) * 100 + diff % 60);
	  }

	default:
	  /* Bad format.  */
	  add (1, *p = *f);
	  break;
	}
    }

  if (p)
    *p = '\0';
  return i;
}
