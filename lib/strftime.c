/* Copyright (C) 1991,92,93,94,95,96,97,98 Free Software Foundation, Inc.

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
# define HAVE_MBRLEN 1
# define HAVE_STRUCT_ERA_ENTRY 1
# define HAVE_TM_GMTOFF 1
# define HAVE_TM_ZONE 1
# define HAVE_TZNAME 1
# define HAVE_TZSET 1
# define MULTIBYTE_IS_FORMAT_SAFE 1
# define STDC_HEADERS 1
# include "../locale/localeinfo.h"
#endif

#if defined emacs && !defined HAVE_BCOPY
# define HAVE_MEMCPY 1
#endif

#include <ctype.h>
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
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#ifdef _LIBC
# define MEMPCPY(d, s, n) __mempcpy (d, s, n)
#else
# ifndef HAVE_MEMPCPY
#  define MEMPCPY(d, s, n) ((void *) ((char *) memcpy (d, s, n) + (n)))
# endif
#endif

#ifndef __P
# if defined (__GNUC__) || (defined (__STDC__) && __STDC__)
#  define __P(args) args
# else
#  define __P(args) ()
# endif  /* GCC.  */
#endif  /* Not __P.  */

#ifndef PTR
# ifdef __STDC__
#  define PTR void *
# else
#  define PTR char *
# endif
#endif

#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif

#ifndef NULL
# define NULL 0
#endif

#define TYPE_SIGNED(t) ((t) -1 < 0)

/* Bound on length of the string representing an integer value of type t.
   Subtract one for the sign bit if t is signed;
   302 / 1000 is log10 (2) rounded up;
   add one for integer division truncation;
   add one more for a minus sign if t is signed.  */
#define INT_STRLEN_BOUND(t) \
 ((sizeof (t) * CHAR_BIT - TYPE_SIGNED (t)) * 302 / 1000 + 1 + TYPE_SIGNED (t))

#define TM_YEAR_BASE 1900

#ifndef __isleap
/* Nonzero if YEAR is a leap year (every 4 years,
   except every 100th isn't, and every 400th is).  */
# define __isleap(year)	\
  ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))
#endif


#ifdef _LIBC
# define my_strftime_gmtime_r __gmtime_r
# define my_strftime_localtime_r __localtime_r
# define tzname __tzname
# define tzset __tzset
#else

/* If we're a strftime substitute in a GNU program, then prefer gmtime
   to gmtime_r, since many gmtime_r implementations are buggy.
   Similarly for localtime_r.  */

# if ! HAVE_TM_GMTOFF
static struct tm *my_strftime_gmtime_r __P ((const time_t *, struct tm *));
static struct tm *
my_strftime_gmtime_r (const time_t *t, struct tm *tp)
{
  struct tm *l = gmtime (t);
  if (! l)
    return 0;
  *tp = *l;
  return tp;
}
# endif /* ! HAVE_TM_GMTOFF */

static struct tm *my_strftime_localtime_r __P ((const time_t *, struct tm *));
static struct tm *
my_strftime_localtime_r (const time_t *t, struct tm *tp)
{
  struct tm *l = localtime (t);
  if (! l)
    return 0;
  *tp = *l;
  return tp;
}
#endif /* ! defined _LIBC */


#if !defined memset && !defined HAVE_MEMSET && !defined _LIBC
/* Some systems lack the `memset' function and we don't want to
   introduce additional dependencies.  */
/* The SGI compiler reportedly barfs on the trailing null
   if we use a string constant as the initializer.  28 June 1997, rms.  */
static const char spaces[16] = /* "                " */
  { ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
static const char zeroes[16] = /* "0000000000000000" */
  { '0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0' };

# define memset_space(P, Len) \
  do {									      \
    int _len = (Len);							      \
									      \
    do									      \
      {									      \
	int _this = _len > 16 ? 16 : _len;				      \
	(P) = MEMPCPY ((P), spaces, _this);				      \
	_len -= _this;							      \
      }									      \
    while (_len > 0);							      \
  } while (0)

# define memset_zero(P, Len) \
  do {									      \
    int _len = (Len);							      \
									      \
    do									      \
      {									      \
	int _this = _len > 16 ? 16 : _len;				      \
	(P) = MEMPCPY ((P), zeroes, _this);				      \
	_len -= _this;							      \
      }									      \
    while (_len > 0);							      \
  } while (0)
#else
# define memset_space(P, Len) (memset ((P), ' ', (Len)), (P) += (Len))
# define memset_zero(P, Len) (memset ((P), '0', (Len)), (P) += (Len))
#endif

#define add(n, f)							      \
  do									      \
    {									      \
      int _n = (n);							      \
      int _delta = width - _n;						      \
      int _incr = _n + (_delta > 0 ? _delta : 0);			      \
      if (i + _incr >= maxsize)						      \
	return 0;							      \
      if (p)								      \
	{								      \
	  if (_delta > 0)						      \
	    {								      \
	      if (pad == '0')						      \
		memset_zero (p, _delta);				      \
	      else							      \
		memset_space (p, _delta);				      \
	    }								      \
	  f;								      \
	  p += _n;							      \
	}								      \
      i += _incr;							      \
    } while (0)

#define cpy(n, s) \
    add ((n),								      \
	 if (to_lowcase)						      \
	   memcpy_lowcase (p, (s), _n);					      \
	 else if (to_uppcase)						      \
	   memcpy_uppcase (p, (s), _n);					      \
	 else								      \
	   memcpy ((PTR) p, (PTR) (s), _n))



#ifdef _LIBC
# define TOUPPER(Ch) toupper (Ch)
# define TOLOWER(Ch) tolower (Ch)
#else
# define TOUPPER(Ch) (islower (Ch) ? toupper (Ch) : (Ch))
# define TOLOWER(Ch) (isupper (Ch) ? tolower (Ch) : (Ch))
#endif
/* We don't use `isdigit' here since the locale dependent
   interpretation is not what we want here.  We only need to accept
   the arabic digits in the ASCII range.  One day there is perhaps a
   more reliable way to accept other sets of digits.  */
#define ISDIGIT(Ch) ((unsigned int) (Ch) - '0' <= 9)

static char *memcpy_lowcase __P ((char *dest, const char *src, size_t len));

static char *
memcpy_lowcase (char *dest, const char *src, size_t len)
{
  while (len-- > 0)
    dest[len] = TOLOWER ((unsigned char) src[len]);
  return dest;
}

static char *memcpy_uppcase __P ((char *dest, const char *src, size_t len));

static char *
memcpy_uppcase (char *dest, const char *src, size_t len)
{
  while (len-- > 0)
    dest[len] = TOUPPER ((unsigned char) src[len]);
  return dest;
}


#if ! HAVE_TM_GMTOFF
/* Yield the difference between *A and *B,
   measured in seconds, ignoring leap seconds.  */
# define tm_diff ftime_tm_diff
static int tm_diff __P ((const struct tm *, const struct tm *));
static int
tm_diff (const struct tm *a, const struct tm *b)
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
__inline__
#endif
static int
iso_week_days (int yday, int wday)
{
  /* Add enough to the first operand of % to make it nonnegative.  */
  int big_enough_multiple_of_7 = (-YDAY_MINIMUM / 7 + 2) * 7;
  return (yday
	  - (yday - wday + ISO_WEEK1_WDAY + big_enough_multiple_of_7) % 7
	  + ISO_WEEK1_WDAY - ISO_WEEK_START_WDAY);
}


#if !(defined _NL_CURRENT || HAVE_STRFTIME)
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


#ifdef emacs
# define my_strftime emacs_strftimeu
# define ut_argument , ut
# define ut_argument_spec int ut;
# define ut_argument_spec_iso , int ut
#else
# define my_strftime strftime
# define ut_argument
# define ut_argument_spec
# define ut_argument_spec_iso
/* We don't have this information in general.  */
# define ut 0
#endif

#if !defined _LIBC && HAVE_TZNAME && HAVE_TZSET
  /* Solaris 2.5 tzset sometimes modifies the storage returned by localtime.
     Work around this bug by copying *tp before it might be munged.  */
  size_t _strftime_copytm __P ((char *, size_t, const char *,
			        const struct tm * ut_argument_spec_iso));
  size_t
  my_strftime (s, maxsize, format, tp ut_argument)
      char *s;
      size_t maxsize;
      const char *format;
      const struct tm *tp;
      ut_argument_spec
  {
    struct tm tmcopy;
    tmcopy = *tp;
    return _strftime_copytm (s, maxsize, format, &tmcopy ut_argument);
  }
# undef my_strftime
# define my_strftime(S, Maxsize, Format, Tp) \
  _strftime_copytm (S, Maxsize, Format, Tp)
#endif


/* Write information from TP into S according to the format
   string FORMAT, writing no more that MAXSIZE characters
   (including the terminating '\0') and returning number of
   characters written.  If S is NULL, nothing will be written
   anywhere, so to determine how many characters would be
   written, use NULL for S and (size_t) UINT_MAX for MAXSIZE.  */
size_t
my_strftime (char *s, size_t maxsize, const char *format, const struct tm *tp
	     ut_argument_spec)
{
  int hour12 = tp->tm_hour;
#ifdef _NL_CURRENT
  /* We cannot make the following values variables since we must delay
     the evaluation of these values until really needed since some
     expressions might not be valid in every situation.  The `struct tm'
     might be generated by a strptime() call that initialized
     only a few elements.  Dereference the pointers only if the format
     requires this.  Then it is ok to fail if the pointers are invalid.  */
# define a_wkday _NL_CURRENT (LC_TIME, ABDAY_1 + tp->tm_wday)
# define f_wkday _NL_CURRENT (LC_TIME, DAY_1 + tp->tm_wday)
# define a_month _NL_CURRENT (LC_TIME, ABMON_1 + tp->tm_mon)
# define f_month _NL_CURRENT (LC_TIME, MON_1 + tp->tm_mon)
# define ampm _NL_CURRENT (LC_TIME, tp->tm_hour > 11 ? PM_STR : AM_STR)

# define aw_len strlen (a_wkday)
# define am_len strlen (a_month)
# define ap_len strlen (ampm)
#else
# if !HAVE_STRFTIME
#  define f_wkday (weekday_name[tp->tm_wday])
#  define f_month (month_name[tp->tm_mon])
#  define a_wkday f_wkday
#  define a_month f_month
#  define ampm ("AMPM" + 2 * (tp->tm_hour > 11))

  size_t aw_len = 3;
  size_t am_len = 3;
  size_t ap_len = 2;
# endif
#endif
  const char *zone;
  size_t i = 0;
  char *p = s;
  const char *f;

  zone = NULL;
#if HAVE_TM_ZONE
  /* The POSIX test suite assumes that setting
     the environment variable TZ to a new value before calling strftime()
     will influence the result (the %Z format) even if the information in
     TP is computed with a totally different time zone.
     This is bogus: though POSIX allows bad behavior like this,
     POSIX does not require it.  Do the right thing instead.  */
  zone = (const char *) tp->tm_zone;
#endif
#if HAVE_TZNAME
  if (ut)
    {
      if (! (zone && *zone))
	zone = "GMT";
    }
  else
    {
      /* POSIX.1 8.1.1 requires that whenever strftime() is called, the
	 time zone names contained in the external variable `tzname' shall
	 be set as if the tzset() function had been called.  */
# if HAVE_TZSET
      tzset ();
# endif
    }
#endif

  if (hour12 > 12)
    hour12 -= 12;
  else
    if (hour12 == 0)
      hour12 = 12;

  for (f = format; *f != '\0'; ++f)
    {
      int pad = 0;		/* Padding for number ('-', '_', or 0).  */
      int modifier;		/* Field modifier ('E', 'O', or 0).  */
      int digits;		/* Max digits for numeric format.  */
      int number_value; 	/* Numeric value to be printed.  */
      int negative_number;	/* 1 if the number is negative.  */
      const char *subfmt;
      char *bufp;
      char buf[1 + (sizeof (int) < sizeof (time_t)
		    ? INT_STRLEN_BOUND (time_t)
		    : INT_STRLEN_BOUND (int))];
      int width = -1;
      int to_lowcase = 0;
      int to_uppcase = 0;
      int change_case = 0;
      int format_char;

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

		if (bytes == (size_t) -2)
		  {
		    len += strlen (f + len);
		    break;
		  }

		if (bytes == (size_t) -1)
		  {
		    len++;
		    break;
		  }

		len += bytes;
	      }
	    while (! mbsinit (&mbstate));

	    cpy (len, f);
	    f += len - 1;
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

      /* Check for flags that can modify a format.  */
      while (1)
	{
	  switch (*++f)
	    {
	      /* This influences the number formats.  */
	    case '_':
	    case '-':
	    case '0':
	      pad = *f;
	      continue;

	      /* This changes textual output.  */
	    case '^':
	      to_uppcase = 1;
	      continue;
	    case '#':
	      change_case = 1;
	      continue;

	    default:
	      break;
	    }
	  break;
	}

      /* As a GNU extension we allow to specify the field width.  */
      if (ISDIGIT (*f))
	{
	  width = 0;
	  do
	    {
	      width *= 10;
	      width += *f - '0';
	      ++f;
	    }
	  while (ISDIGIT (*f));
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
      format_char = *f;
      switch (format_char)
	{
#define DO_NUMBER(d, v) \
	  digits = width == -1 ? d : width;				      \
	  number_value = v; goto do_number
#define DO_NUMBER_SPACEPAD(d, v) \
	  digits = width == -1 ? d : width;				      \
	  number_value = v; goto do_number_spacepad

	case '%':
	  if (modifier != 0)
	    goto bad_format;
	  add (1, *p = *f);
	  break;

	case 'a':
	  if (modifier != 0)
	    goto bad_format;
	  if (change_case)
	    {
	      to_uppcase = 1;
	      to_lowcase = 0;
	    }
#if defined _NL_CURRENT || !HAVE_STRFTIME
	  cpy (aw_len, a_wkday);
	  break;
#else
	  goto underlying_strftime;
#endif

	case 'A':
	  if (modifier != 0)
	    goto bad_format;
	  if (change_case)
	    {
	      to_uppcase = 1;
	      to_lowcase = 0;
	    }
#if defined _NL_CURRENT || !HAVE_STRFTIME
	  cpy (strlen (f_wkday), f_wkday);
	  break;
#else
	  goto underlying_strftime;
#endif

	case 'b':
	case 'h':		/* POSIX.2 extension.  */
	  if (modifier != 0)
	    goto bad_format;
#if defined _NL_CURRENT || !HAVE_STRFTIME
	  cpy (am_len, a_month);
	  break;
#else
	  goto underlying_strftime;
#endif

	case 'B':
	  if (modifier != 0)
	    goto bad_format;
	  if (change_case)
	    {
	      to_uppcase = 1;
	      to_lowcase = 0;
	    }
#if defined _NL_CURRENT || !HAVE_STRFTIME
	  cpy (strlen (f_month), f_month);
	  break;
#else
	  goto underlying_strftime;
#endif

	case 'c':
	  if (modifier == 'O')
	    goto bad_format;
#ifdef _NL_CURRENT
	  if (! (modifier == 'E'
		 && *(subfmt = _NL_CURRENT (LC_TIME, ERA_D_T_FMT)) != '\0'))
	    subfmt = _NL_CURRENT (LC_TIME, D_T_FMT);
#else
# if HAVE_STRFTIME
	  goto underlying_strftime;
# else
	  subfmt = "%a %b %e %H:%M:%S %Y";
# endif
#endif

	subformat:
	  {
	    char *old_start = p;
	    size_t len = my_strftime (NULL, (size_t) -1, subfmt, tp);
	    add (len, my_strftime (p, maxsize - i, subfmt, tp));

	    if (to_uppcase)
	      while (old_start < p)
		{
		  *old_start = TOUPPER ((unsigned char) *old_start);
		  ++old_start;
		}
	  }
	  break;

#if HAVE_STRFTIME && ! (defined _NL_CURRENT && HAVE_STRUCT_ERA_ENTRY)
	underlying_strftime:
	  {
	    /* The relevant information is available only via the
	       underlying strftime implementation, so use that.  */
	    char ufmt[4];
	    char *u = ufmt;
	    char ubuf[1024]; /* enough for any single format in practice */
	    size_t len;
	    *u++ = '%';
	    if (modifier != 0)
	      *u++ = modifier;
	    *u++ = format_char;
	    *u = '\0';
	    len = strftime (ubuf, sizeof ubuf, ufmt, tp);
	    if (len == 0 && ubuf[0] != '\0')
	      return 0;
	    cpy (len, ubuf);
	  }
	  break;
#endif

	case 'C':		/* POSIX.2 extension.  */
	  if (modifier == 'O')
	    goto bad_format;
	  if (modifier == 'E')
	    {
#if HAVE_STRUCT_ERA_ENTRY
	      struct era_entry *era = _nl_get_era_entry (tp);
	      if (era)
		{
		  size_t len = strlen (era->name_fmt);
		  cpy (len, era->name_fmt);
		  break;
		}
#else
# if HAVE_STRFTIME
	      goto underlying_strftime;
# endif
#endif
	    }

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
#else
# if HAVE_STRFTIME
	  goto underlying_strftime;
# else
	  /* Fall through.  */
# endif
#endif
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
	  /* Force `_' flag unless overwritten by `0' flag.  */
	  if (pad != '0')
	    pad = '_';

	do_number:
	  /* Format the number according to the MODIFIER flag.  */

	  if (modifier == 'O' && 0 <= number_value)
	    {
#ifdef _NL_CURRENT
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
#else
# if HAVE_STRFTIME
	      goto underlying_strftime;
# endif
#endif
	    }
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

	case 'F':
	  if (modifier != 0)
	    goto bad_format;
	  subfmt = "%Y-%m-%d";
	  goto subformat;

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

	case 'P':
	  to_lowcase = 1;
#if !defined _NL_CURRENT && HAVE_STRFTIME
	  format_char = 'p';
#endif
	  /* FALLTHROUGH */

	case 'p':
	  if (change_case)
	    {
	      to_uppcase = 0;
	      to_lowcase = 1;
	    }
#if defined _NL_CURRENT || !HAVE_STRFTIME
	  cpy (ap_len, ampm);
	  break;
#else
	  goto underlying_strftime;
#endif

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
#else
# if HAVE_STRFTIME
	  goto underlying_strftime;
# else
	  /* Fall through.  */
# endif
#endif
	case 'T':		/* POSIX.2 extension.  */
	  subfmt = "%H:%M:%S";
	  goto subformat;

	case 't':		/* POSIX.2 extension.  */
	  add (1, *p = '\t');
	  break;

	case 'f':
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
	  if (modifier == 'E')
	    {
#if HAVE_STRUCT_ERA_ENTRY
	      struct era_entry *era = _nl_get_era_entry (tp);
	      if (era)
		{
		  subfmt = strchr (era->name_fmt, '\0') + 1;
		  goto subformat;
		}
#else
# if HAVE_STRFTIME
	      goto underlying_strftime;
# endif
#endif
	    }
	  if (modifier == 'O')
	    goto bad_format;
	  else
	    DO_NUMBER (1, tp->tm_year + TM_YEAR_BASE);

	case 'y':
	  if (modifier == 'E')
	    {
#if HAVE_STRUCT_ERA_ENTRY
	      struct era_entry *era = _nl_get_era_entry (tp);
	      if (era)
		{
		  int delta = tp->tm_year - era->start_date[0];
		  DO_NUMBER (1, (era->offset
				 + (era->direction == '-' ? -delta : delta)));
		}
#else
# if HAVE_STRFTIME
	      goto underlying_strftime;
# endif
#endif
	    }
	  DO_NUMBER (2, (tp->tm_year % 100 + 100) % 100);

	case 'Z':
	  if (change_case)
	    {
	      to_uppcase = 0;
	      to_lowcase = 1;
	    }

#if HAVE_TZNAME
	  /* The tzset() call might have changed the value.  */
	  if (!(zone && *zone) && tp->tm_isdst >= 0)
	    zone = tzname[tp->tm_isdst];
#endif
	  if (! zone)
	    zone = "";		/* POSIX.2 requires the empty string here.  */

	  cpy (strlen (zone), zone);
	  break;

	case 'z':		/* GNU extension.  */
	  if (tp->tm_isdst < 0)
	    break;

	  {
	    int diff;
#if HAVE_TM_GMTOFF
	    diff = tp->tm_gmtoff;
#else
	    if (ut)
	      diff = 0;
	    else
	      {
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

		    if (! my_strftime_localtime_r (&lt, &tm)
			|| ((ltm.tm_sec ^ tm.tm_sec)
			    | (ltm.tm_min ^ tm.tm_min)
			    | (ltm.tm_hour ^ tm.tm_hour)
			    | (ltm.tm_mday ^ tm.tm_mday)
			    | (ltm.tm_mon ^ tm.tm_mon)
			    | (ltm.tm_year ^ tm.tm_year)))
		      break;
		  }

		if (! my_strftime_gmtime_r (&lt, &gtm))
		  break;

		diff = tm_diff (&ltm, &gtm);
	      }
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

  if (p && i < maxsize)
    *p = '\0';
  return i;
}


#ifdef emacs
/* For Emacs we have a separate interface which corresponds to the normal
   strftime function and does not have the extra information whether the
   TP arguments comes from a `gmtime' call or not.  */
size_t
emacs_strftime (s, maxsize, format, tp)
      char *s;
      size_t maxsize;
      const char *format;
      const struct tm *tp;
{
  return my_strftime (s, maxsize, format, tp, 0);
}
#endif
