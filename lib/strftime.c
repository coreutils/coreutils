/* Copyright (C) 1991, 92, 93, 94, 95, 96 Free Software Foundation, Inc.

   NOTE: The canonical source of this file is maintained with the GNU C
   Library.  Bugs can be reported to bug-glibc@prep.ai.mit.edu.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef _LIBC
# define HAVE_LIMITS_H 1
# define HAVE_MBLEN 1
# define HAVE_MBRLEN 1
# define HAVE_STRUCT_ERA_ENTRY 1
# define HAVE_TM_GMTOFF 1
# define HAVE_TM_ZONE 1
# define MULTIBYTE_IS_FORMAT_SAFE 1
# define STDC_HEADERS 1
# include <ansidecl.h>
# include "../locale/localeinfo.h"
#endif

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

#if HAVE_TZNAME
extern char *tzname[];
#endif

/* Do multibyte processing if multibytes are supported, unless
   multibyte sequences are safe in formats.  Multibyte sequences are
   safe if they cannot contain byte sequences that look like format
   conversion specifications.  The GNU C Library uses UTF8 multibyte
   encoding, which is safe for formats, but strftime.c can be used
   with other C libraries that use unsafe encodings.  */
#define DO_MULTIBYTE (HAVE_MBLEN && ! MULTIBYTE_IS_FORMAT_SAFE)

#if DO_MULTIBYTE
# if HAVE_MBRLEN
#  include <wchar.h>
# else
   /* Simulate mbrlen with mblen as best we can.  */
#  define mbstate_t int
#  define mbrlen(s, n, ps) mblen (s, n)
#  define mbsinit(ps) (*(ps) == 0)
# endif
  static const mbstate_t mbstate_zero;
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

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#define TYPE_SIGNED(t) ((t) -1 < 0)

/* Bound on length of the string representing an integer value of type t.
   Subtract one for the sign bit if t is signed;
   302 / 1000 is log10 (2) rounded up;
   add one for integer division truncation;
   add one more for a minus sign if t is signed.  */
#define INT_STRLEN_BOUND(t) \
  ((sizeof (t) * CHAR_BIT - TYPE_SIGNED (t)) * 302 / 100 + 1 + TYPE_SIGNED (t))

#define TM_YEAR_BASE 1900

#ifndef __isleap
/* Nonzero if YEAR is a leap year (every 4 years,
   except every 100th isn't, and every 400th is).  */
#define __isleap(year)	\
  ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))
#endif


#ifdef _LIBC
# define gmtime_r __gmtime_r
# define localtime_r __localtime_r
#else
# if ! HAVE_LOCALTIME_R
#  if ! HAVE_TM_GMTOFF
/* Approximate gmtime_r as best we can in its absence.  */
#define gmtime_r my_gmtime_r
static struct tm *gmtime_r __P ((const time_t *, struct tm *));
static struct tm *
gmtime_r (t, tp)
     const time_t *t;
     struct tm *tp;
{
  struct tm *l = gmtime (t);
  if (! l)
    return 0;
  *tp = *l;
  return tp;
}
#  endif /* ! HAVE_TM_GMTOFF */

/* Approximate localtime_r as best we can in its absence.  */
#define localtime_r my_localtime_r
static struct tm *localtime_r __P ((const time_t *, struct tm *));
static struct tm *
localtime_r (t, tp)
     const time_t *t;
     struct tm *tp;
{
  struct tm *l = localtime (t);
  if (! l)
    return 0;
  *tp = *l;
  return tp;
}
# endif /* ! HAVE_LOCALTIME_R */
#endif /* ! defined (_LIBC) */


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

#if ! HAVE_TM_GMTOFF
/* Yield the difference between *A and *B,
   measured in seconds, ignoring leap seconds.  */
static int tm_diff __P ((const struct tm *, const struct tm *));
static int
tm_diff (a, b)
     const struct tm *a;
     const struct tm *b;
{
  /* Compute intervening leap days correctly even if year is negative.
     Take care to avoid int overflow in leap day calculations,
     but it's OK to assume that A and B are close to each other.  */
  int a4 = (a->tm_year >> 2) + (TM_YEAR_BASE >> 2) - ! (a->tm_year & 3);
  int b4 = (b->tm_year >> 2) + (TM_YEAR_BASE >> 2) - ! (b->tm_year & 3);
  int a100 = a4 / 25 - (a4 % 25 < 0);
  int b100 = b4 / 25 - (b4 % 25 < 0);
  int a400 = a100 >> 2;
  int b400 = b100 >> 2;
  int intervening_leap_days = (a4 - b4) - (a100 - b100) + (a400 - b400);
  int years = a->tm_year - b->tm_year;
  int days = (365 * years + intervening_leap_days
	      + (a->tm_yday - b->tm_yday));
  return (60 * (60 * (24 * days + (a->tm_hour - b->tm_hour))
		+ (a->tm_min - b->tm_min))
	  + (a->tm_sec - b->tm_sec));
}
#endif /* ! HAVE_TM_GMTOFF */



/* The number of days from the first day of the first ISO week of this
   year to the year day YDAY with week day WDAY.  ISO weeks start on
   Monday; the first ISO week has the year's first Thursday.  YDAY may
   be as small as YDAY_MINIMUM.  */
#define ISO_WEEK_START_WDAY 1 /* Monday */
#define ISO_WEEK1_WDAY 4 /* Thursday */
#define YDAY_MINIMUM (-366)
static int iso_week_days __P ((int, int));
#ifdef __GNUC__
inline
#endif
static int
iso_week_days (yday, wday)
     int yday;
     int wday;
{
  /* Add enough to the first operand of % to make it nonnegative.  */
  int big_enough_multiple_of_7 = (-YDAY_MINIMUM / 7 + 2) * 7;
  return (yday
	  - (yday - wday + ISO_WEEK1_WDAY + big_enough_multiple_of_7) % 7
	  + ISO_WEEK1_WDAY - ISO_WEEK_START_WDAY);
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
  size_t aw_len = strlen (a_wkday);
  size_t am_len = strlen (a_month);
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
  const char *zone;
  size_t zonelen;
  register size_t i = 0;
  register char *p = s;
  register const char *f;

  zone = 0;
#if HAVE_TM_ZONE
  zone = (const char *) tp->tm_zone;
#endif
#if HAVE_TZNAME
  if (!(zone && *zone) && tp->tm_isdst >= 0)
    zone = tzname[tp->tm_isdst];
#endif
  if (! zone)
    zone = "";		/* POSIX.2 requires the empty string here.  */

  zonelen = strlen (zone);

  if (hour12 > 12)
    hour12 -= 12;
  else
    if (hour12 == 0) hour12 = 12;

  for (f = format; *f != '\0'; ++f)
    {
      int pad;			/* Padding for number ('-', '_', or 0).  */
      int modifier;		/* Field modifier ('E', 'O', or 0).  */
      int digits;		/* Max digits for numeric format.  */
      int number_value; 	/* Numeric value to be printed.  */
      int negative_number;	/* 1 if the number is negative.  */
      const char *subfmt;
      char *bufp;
      char buf[1 + (sizeof (int) < sizeof (time_t)
		    ? INT_STRLEN_BOUND (time_t)
		    : INT_STRLEN_BOUND (int))];

#if DO_MULTIBYTE

       switch (*f)
	{
	case '%':
	  break;

	case '\a': case '\b': case '\t': case '\n':
	case '\v': case '\f': case '\r':
	case ' ': case '!': case '"': case '#': case '&': case'\'':
	case '(': case ')': case '*': case '+': case ',': case '-':
	case '.': case '/': case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7': case '8': case '9':
	case ':': case ';': case '<': case '=': case '>': case '?':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
	case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
	case 'Y': case 'Z': case '[': case'\\': case ']': case '^':
	case '_': case 'a': case 'b': case 'c': case 'd': case 'e':
	case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
	case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
	case 'r': case 's': case 't': case 'u': case 'v': case 'w':
	case 'x': case 'y': case 'z': case '{': case '|': case '}':
	case '~':
	  /* The C Standard requires these 98 characters (plus '%') to
	     be in the basic execution character set.  None of these
	     characters can start a multibyte sequence, so they need
	     not be analyzed further.  */
	  add (1, *p = *f);
	  continue;

	default:
	  /* Copy this multibyte sequence until we reach its end, find
	     an error, or come back to the initial shift state.  */
	  {
	    mbstate_t mbstate = mbstate_zero;
	    size_t len = 0;

	    do
	      {
		size_t bytes = mbrlen (f + len, (size_t) -1, &mbstate);

		if (bytes == 0)
		  break;

		if (bytes == (size_t) -2 || bytes == (size_t) -1)
		  {
		    len++;
		    break;
		  }

		len += bytes;
	      }
	    while (! mbsinit (&mbstate));

	    cpy (len, f);
	    continue;
	  }
	}

#else /* ! DO_MULTIBYTE */

      /* Either multibyte encodings are not supported, or they are
	 safe for formats, so any non-'%' byte can be copied through.  */
      if (*f != '%')
	{
	  add (1, *p = *f);
	  continue;
	}

#endif /* ! DO_MULTIBYTE */

      /* Check for flags that can modify a number format.  */
      ++f;
      switch (*f)
	{
	case '_':
	case '-':
	  pad = *f++;
	  break;

	default:
	  pad = 0;
	  break;
	}

      /* Check for modifiers.  */
      switch (*f)
	{
	case 'E':
	case 'O':
	  modifier = *f++;
	  break;

	default:
	  modifier = 0;
	  break;
	}

      /* Now do the specified format.  */
      switch (*f)
	{
#define DO_NUMBER(d, v) \
	  digits = d; number_value = v; goto do_number
#define DO_NUMBER_SPACEPAD(d, v) \
	  digits = d; number_value = v; goto do_number_spacepad

	case '%':
	  if (modifier != 0)
	    goto bad_format;
	  add (1, *p = *f);
	  break;

	case 'a':
	  if (modifier != 0)
	    goto bad_format;
	  cpy (aw_len, a_wkday);
	  break;

	case 'A':
	  if (modifier != 0)
	    goto bad_format;
	  cpy (wkday_len, f_wkday);
	  break;

	case 'b':
	case 'h':		/* POSIX.2 extension.  */
	  if (modifier != 0)
	    goto bad_format;
	  cpy (am_len, a_month);
	  break;

	case 'B':
	  if (modifier != 0)
	    goto bad_format;
	  cpy (month_len, f_month);
	  break;

	case 'c':
	  if (modifier == 'O')
	    goto bad_format;
#ifdef _NL_CURRENT
	  if (! (modifier == 'E'
		 && *(subfmt = _NL_CURRENT (LC_TIME, ERA_D_T_FMT)) != '\0'))
	    subfmt = _NL_CURRENT (LC_TIME, D_T_FMT);
#else
	  subfmt = "%a %b %e %H:%M:%S %Y";
#endif

	subformat:
	  {
	    size_t len = strftime (p, maxsize - i, subfmt, tp);
	    if (len == 0 && *subfmt)
	      return 0;
	    add (len, ;);
	  }
	  break;

	case 'C':		/* POSIX.2 extension.  */
	  if (modifier == 'O')
	    goto bad_format;
#if HAVE_STRUCT_ERA_ENTRY
	  if (modifier == 'E')
	    {
	      struct era_entry *era = _nl_get_era_entry (tp);
	      if (era)
		{
		  size_t len = strlen (era->name_fmt);
		  cpy (len, era->name_fmt);
		  break;
		}
	    }
#endif
	  {
	    int year = tp->tm_year + TM_YEAR_BASE;
	    DO_NUMBER (1, year / 100 - (year % 100 < 0));
	  }

	case 'x':
	  if (modifier == 'O')
	    goto bad_format;
#ifdef _NL_CURRENT
	  if (! (modifier == 'E'
		 && *(subfmt = _NL_CURRENT (LC_TIME, ERA_D_FMT)) != '\0'))
	    subfmt = _NL_CURRENT (LC_TIME, D_FMT);
	  goto subformat;
#endif
	  /* Fall through.  */
	case 'D':		/* POSIX.2 extension.  */
	  if (modifier != 0)
	    goto bad_format;
	  subfmt = "%m/%d/%y";
	  goto subformat;

	case 'd':
	  if (modifier == 'E')
	    goto bad_format;

	  DO_NUMBER (2, tp->tm_mday);

	case 'e':		/* POSIX.2 extension.  */
	  if (modifier == 'E')
	    goto bad_format;

	  DO_NUMBER_SPACEPAD (2, tp->tm_mday);

	  /* All numeric formats set DIGITS and NUMBER_VALUE and then
	     jump to one of these two labels.  */

	do_number_spacepad:
	  /* Force `_' flag.  */
	  pad = '_';

	do_number:
	  /* Format the number according to the MODIFIER flag.  */

#ifdef _NL_CURRENT
	  if (modifier == 'O' && 0 <= number_value)
	    {
	      /* Get the locale specific alternate representation of
		 the number NUMBER_VALUE.  If none exist NULL is returned.  */
	      const char *cp = _nl_get_alt_digit (number_value);

	      if (cp != NULL)
		{
		  size_t digitlen = strlen (cp);
		  if (digitlen != 0)
		    {
		      cpy (digitlen, cp);
		      break;
		    }
		}
	    }
#endif
	  {
	    unsigned int u = number_value;

	    bufp = buf + sizeof (buf);
	    negative_number = number_value < 0;

	    if (negative_number)
	      u = -u;

	    do
	      *--bufp = u % 10 + '0';
	    while ((u /= 10) != 0);
  	  }

	do_number_sign_and_padding:
	  if (negative_number)
	    *--bufp = '-';

	  if (pad != '-')
	    {
	      int padding = digits - (buf + sizeof (buf) - bufp);

	      if (pad == '_')
		{
		  while (0 < padding--)
		    *--bufp = ' ';
		}
	      else
		{
		  bufp += negative_number;
		  while (0 < padding--)
		    *--bufp = '0';
		  if (negative_number)
		    *--bufp = '-';
		}
	    }

	  cpy (buf + sizeof (buf) - bufp, bufp);
	  break;


	case 'H':
	  if (modifier == 'E')
	    goto bad_format;

	  DO_NUMBER (2, tp->tm_hour);

	case 'I':
	  if (modifier == 'E')
	    goto bad_format;

	  DO_NUMBER (2, hour12);

	case 'k':		/* GNU extension.  */
	  if (modifier == 'E')
	    goto bad_format;

	  DO_NUMBER_SPACEPAD (2, tp->tm_hour);

	case 'l':		/* GNU extension.  */
	  if (modifier == 'E')
	    goto bad_format;

	  DO_NUMBER_SPACEPAD (2, hour12);

	case 'j':
	  if (modifier == 'E')
	    goto bad_format;

	  DO_NUMBER (3, 1 + tp->tm_yday);

	case 'M':
	  if (modifier == 'E')
	    goto bad_format;

	  DO_NUMBER (2, tp->tm_min);

	case 'm':
	  if (modifier == 'E')
	    goto bad_format;

	  DO_NUMBER (2, tp->tm_mon + 1);

	case 'n':		/* POSIX.2 extension.  */
	  add (1, *p = '\n');
	  break;

	case 'p':
	  cpy (ap_len, ampm);
	  break;

	case 'R':		/* GNU extension.  */
	  subfmt = "%H:%M";
	  goto subformat;

	case 'r':		/* POSIX.2 extension.  */
#ifdef _NL_CURRENT
	  if (*(subfmt = _NL_CURRENT (LC_TIME, T_FMT_AMPM)) == '\0')
#endif
	    subfmt = "%I:%M:%S %p";
	  goto subformat;

	case 'S':
	  if (modifier == 'E')
	    goto bad_format;

	  DO_NUMBER (2, tp->tm_sec);

	case 's':		/* GNU extension.  */
  	  {
	    struct tm ltm;
	    time_t t;

	    ltm = *tp;
	    t = mktime (&ltm);

	    /* Generate string value for T using time_t arithmetic;
	       this works even if sizeof (long) < sizeof (time_t).  */

	    bufp = buf + sizeof (buf);
	    negative_number = t < 0;

	    do
	      {
		int d = t % 10;
		t /= 10;

		if (negative_number)
		  {
		    d = -d;

		    /* Adjust if division truncates to minus infinity.  */
		    if (0 < -1 % 10 && d < 0)
		      {
			t++;
			d += 10;
		      }
		  }

		*--bufp = d + '0';
	      }
	    while (t != 0);

	    digits = 1;
	    goto do_number_sign_and_padding;
	  }

	case 'X':
	  if (modifier == 'O')
	    goto bad_format;
#ifdef _NL_CURRENT
	  if (! (modifier == 'E'
		 && *(subfmt = _NL_CURRENT (LC_TIME, ERA_T_FMT)) != '\0'))
	    subfmt = _NL_CURRENT (LC_TIME, T_FMT);
	  goto subformat;
#endif
	  /* Fall through.  */
	case 'T':		/* POSIX.2 extension.  */
	  subfmt = "%H:%M:%S";
	  goto subformat;

	case 't':		/* POSIX.2 extension.  */
	  add (1, *p = '\t');
	  break;

	case 'u':		/* POSIX.2 extension.  */
	  DO_NUMBER (1, (tp->tm_wday - 1 + 7) % 7 + 1);

	case 'U':
	  if (modifier == 'E')
	    goto bad_format;

	  DO_NUMBER (2, (tp->tm_yday - tp->tm_wday + 7) / 7);

	case 'V':
	case 'g':		/* GNU extension.  */
	case 'G':		/* GNU extension.  */
	  if (modifier == 'E')
	    goto bad_format;
	  {
	    int year = tp->tm_year + TM_YEAR_BASE;
	    int days = iso_week_days (tp->tm_yday, tp->tm_wday);

	    if (days < 0)
	      {
		/* This ISO week belongs to the previous year.  */
		year--;
		days = iso_week_days (tp->tm_yday + (365 + __isleap (year)),
				      tp->tm_wday);
	      }
	    else
	      {
		int d = iso_week_days (tp->tm_yday - (365 + __isleap (year)),
				       tp->tm_wday);
		if (0 <= d)
		  {
		    /* This ISO week belongs to the next year.  */
		    year++;
		    days = d;
		  }
	      }

	    switch (*f)
	      {
	      case 'g':
		DO_NUMBER (2, (year % 100 + 100) % 100);

	      case 'G':
		DO_NUMBER (1, year);

	      default:
		DO_NUMBER (2, days / 7 + 1);
	      }
	  }

	case 'W':
	  if (modifier == 'E')
	    goto bad_format;

	  DO_NUMBER (2, (tp->tm_yday - (tp->tm_wday - 1 + 7) % 7 + 7) / 7);

	case 'w':
	  if (modifier == 'E')
	    goto bad_format;

	  DO_NUMBER (1, tp->tm_wday);

	case 'Y':
#if HAVE_STRUCT_ERA_ENTRY
	  if (modifier == 'E')
	    {
	      struct era_entry *era = _nl_get_era_entry (tp);
	      if (era)
		{
		  subfmt = strchr (era->name_fmt, '\0') + 1;
		  goto subformat;
		}
	    }
#endif
	  if (modifier == 'O')
	    goto bad_format;
	  else
	    DO_NUMBER (1, tp->tm_year + TM_YEAR_BASE);

	case 'y':
#if HAVE_STRUCT_ERA_ENTRY
	  if (modifier == 'E')
	    {
	      struct era_entry *era = _nl_get_era_entry (tp);
	      if (era)
		{
		  int delta = tp->tm_year - era->start_date[0];
		  DO_NUMBER (1, (era->offset
				 + (era->direction == '-' ? -delta : delta)));
		}
	    }
#endif
	  DO_NUMBER (2, (tp->tm_year % 100 + 100) % 100);

	case 'Z':
	  cpy (zonelen, zone);
	  break;

	case 'z':		/* GNU extension.  */
	  if (tp->tm_isdst < 0)
	    break;

	  {
	    int diff;
#if HAVE_TM_GMTOFF
	    diff = tp->tm_gmtoff;
#else
	    struct tm gtm;
	    struct tm ltm;
	    time_t lt;

	    ltm = *tp;
	    lt = mktime (&ltm);

	    if (lt == (time_t) -1)
	      {
		/* mktime returns -1 for errors, but -1 is also a
		   valid time_t value.  Check whether an error really
		   occurred.  */
		struct tm tm;
		localtime_r (&lt, &tm);

		if ((ltm.tm_sec ^ tm.tm_sec)
		    | (ltm.tm_min ^ tm.tm_min)
		    | (ltm.tm_hour ^ tm.tm_hour)
		    | (ltm.tm_mday ^ tm.tm_mday)
		    | (ltm.tm_mon ^ tm.tm_mon)
		    | (ltm.tm_year ^ tm.tm_year))
		  break;
	      }

	    if (! gmtime_r (&lt, &gtm))
	      break;

	    diff = tm_diff (&ltm, &gtm);
#endif

	    if (diff < 0)
	      {
		add (1, *p = '-');
		diff = -diff;
	      }
	    else
	      add (1, *p = '+');

	    diff /= 60;
	    DO_NUMBER (4, (diff / 60) * 100 + diff % 60);
	  }

	case '\0':		/* GNU extension: % at end of format.  */
	    --f;
	    /* Fall through.  */
	default:
	  /* Unknown format; output the format, including the '%',
	     since this is most likely the right thing to do if a
	     multibyte string has been misparsed.  */
	bad_format:
	  {
	    int flen;
	    for (flen = 1; f[1 - flen] != '%'; flen++)
	      continue;
	    cpy (flen, &f[1 - flen]);
	  }
	  break;
	}
    }

  if (p)
    *p = '\0';
  return i;
}
