/* Copyright (C) 1993, 1994 Free Software Foundation, Inc.
   Contributed by Noel Cragg (noel@cs.oberlin.edu).

This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

/* Define this to have a standalone program to test this implementation of
   mktime.  */
/* #define DEBUG */

#ifdef HAVE_CONFIG_H
#if defined (CONFIG_BROKETS)
/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because it found this file in $srcdir).  */
#include <config.h>
#else
#include "config.h"
#endif
#endif

#include <sys/types.h>		/* Some systems define `time_t' here.  */
#include <time.h>


#ifndef __isleap
/* Nonzero if YEAR is a leap year (every 4 years,
   except every 100th isn't, and every 400th is).  */
#define	__isleap(year)	\
  ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))
#endif

#ifndef __P
#if defined (__GNUC__) || (defined (__STDC__) && __STDC__)
#define __P(args) args
#else
#define __P(args) ()
#endif  /* GCC.  */
#endif  /* Not __P.  */

/* How many days are in each month.  */
const unsigned short int __mon_lengths[2][12] =
  {
    /* Normal years.  */
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    /* Leap years.  */
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
  };


static int times_through_search; /* This library routine should never
				    hang -- make sure we always return
				    when we're searching for a value */

/* After testing this, the maximum number of iterations that I had on
   any number that I tried was 3!  Not bad.

   mktime converts a `struct tm' (broken-down local time) into a `time_t';
   it is the opposite of localtime.  It is possible to put the following
   values out of range and have mktime compensate: tm_sec, tm_min, tm_hour,
   tm_mday, tm_year.  The other values in the structure are ignored.  */

#ifdef DEBUG

#include <stdio.h>
#include <ctype.h>

int debugging_enabled = 0;

/* Print the values in a `struct tm'. */
static void
printtm (it)
     struct tm *it;
{
  printf ("%d/%d/%d %d:%d:%d (%s) yday:%d f:%d o:%ld",
	  it->tm_mon,
	  it->tm_mday,
	  it->tm_year,
	  it->tm_hour,
	  it->tm_min,
	  it->tm_sec,
	  it->tm_zone,
	  it->tm_yday,
	  it->tm_isdst,
	  it->tm_gmtoff);
}
#endif


static time_t
dist_tm (t1, t2)
     struct tm *t1;
     struct tm *t2;
{
  time_t distance = 0;
  unsigned long int v1, v2;
  int diff_flag = 0;

  v1 = v2 = 0;

#define doit(x, secs)                                                         \
  v1 += t1->x * secs;                                                         \
  v2 += t2->x * secs;                                                         \
  if (!diff_flag)                                                             \
    {                                                                         \
      if (t1->x < t2->x)                                                      \
	diff_flag = -1;                                                       \
      else if (t1->x > t2->x)                                                 \
	diff_flag = 1;                                                        \
    }
  
  doit (tm_year, 31536000);	/* Okay, not all years have 365 days. */
  doit (tm_mon, 2592000);	/* Okay, not all months have 30 days. */
  doit (tm_mday, 86400);
  doit (tm_hour, 3600);
  doit (tm_min, 60);
  doit (tm_sec, 1);
  
#undef doit
  
  /* We should also make sure that the sign of DISTANCE is correct -- if
     DIFF_FLAG is positive, the distance should be positive and vice versa. */
  
  distance = (v1 > v2) ? (v1 - v2) : (v2 - v1);
  if (diff_flag < 0)
    distance = -distance;

  if (times_through_search > 20) /* Arbitrary # of calls, but makes sure we
				    never hang if there's a problem with
				    this algorithm.  */
    {
      distance = diff_flag;
    }

  /* We need this DIFF_FLAG business because it is forseeable that the
     distance may be zero when, in actuality, the two structures are
     different.  This is usually the case when the dates are 366 days apart
     and one of the years is a leap year.  */

  if (distance == 0 && diff_flag)
    distance = 86400 * diff_flag;

  return distance;
}
      

/* Modified b-search -- make intelligent guesses as to where the time might
   lie along the timeline, assuming that our target time lies a linear
   distance (w/o considering time jumps of a particular region).

   Assume that time does not fluctuate at all along the timeline -- e.g.,
   assume that a day will always take 86400 seconds, etc. -- and come up
   with a hypothetical value for the time_t representation of the struct tm
   TARGET, in relation to the guess variable -- it should be pretty close! */

static time_t
search (target, producer)
     struct tm *target;
     struct tm *(*producer) __P ((const time_t *));
{
  struct tm *guess_tm;
  time_t guess = 0;
  time_t distance = 0;

  times_through_search = 0;

  do
    {
      guess += distance;

      times_through_search++;     
      
      guess_tm = (*producer) (&guess);
      
#ifdef DEBUG
      if (debugging_enabled)
	{
	  printf ("guess %d == ", (int) guess);
	  printtm (guess_tm);
	  puts ("");
	}
#endif
      
      /* Are we on the money? */
      distance = dist_tm (target, guess_tm);
      
    } while (distance != 0);

  return guess;
}

/* Since this function will call localtime many times (and the user might
   be passing their `struct tm *' right from localtime, let's make a copy
   for ourselves and run the search on the copy.

   Also, we have to normalize *TIMEPTR because it's possible to call mktime
   with values that are out of range for a specific item (like Feb 30th). */
time_t
_mktime_internal (timeptr, producer)
     struct tm *timeptr;
     struct tm *(*producer) __P ((const time_t *));
{
  struct tm private_mktime_struct_tm; /* Yes, users can get a ptr to this. */
  struct tm *me;
  time_t result;

  me = &private_mktime_struct_tm;
  
  *me = *timeptr;

#define normalize(foo,x,y,bar); \
  while (me->foo < x) \
    { \
      me->bar--; \
      me->foo = (y - (x - me->foo)); \
    } \
  while (me->foo > y) \
    { \
      me->bar++; \
      me->foo = (x + (me->foo - y)); \
    }
  
  normalize (tm_sec, 0, 59, tm_min);
  normalize (tm_min, 0, 59, tm_hour);
  normalize (tm_hour, 0, 23, tm_mday);
  
  /* Do the month first, so day range can be found. */
  normalize (tm_mon, 0, 11, tm_year);
  normalize (tm_mday, 1,
	     __mon_lengths[__isleap (me->tm_year)][me->tm_mon],
	     tm_mon);

  /* Do the month again, because the day may have pushed it out of range. */
  normalize (tm_mon, 0, 11, tm_year);

  /* Do the day again, because the month may have changed the range. */
  normalize (tm_mday, 1,
	     __mon_lengths[__isleap (me->tm_year)][me->tm_mon],
	     tm_mon);
  
#ifdef DEBUG
  if (debugging_enabled)
    {
      printf ("After normalizing: ");
      printtm (me);
      puts ("\n");
    }
#endif

  result = search (me, producer);

  *timeptr = *me;

  return result;
}

time_t
mktime (timeptr)
     struct tm *timeptr;
{
  return _mktime_internal (timeptr, localtime);
}

#ifdef DEBUG
void
main (argc, argv)
     int argc;
     char *argv[];
{
  int time;
  int result_time;
  struct tm *tmptr;
  
  if (argc == 1)
    {
      long q;
      
      printf ("starting long test...\n");

      for (q = 10000000; q < 1000000000; q++)
	{
	  struct tm *tm = localtime (&q);
	  if ((q % 10000) == 0) { printf ("%ld\n", q); fflush (stdout); }
	  if (q != mktime (tm))
	    { printf ("failed for %ld\n", q); fflush (stdout); }
	}
      
      printf ("test finished\n");

      exit (0);
    }
  
  if (argc != 2)
    {
      printf ("wrong # of args\n");
      exit (0);
    }
  
  debugging_enabled = 1;	/* We want to see the info */

  ++argv;
  time = atoi (*argv);
  
  printf ("Time: %d %s\n", time, ctime ((time_t *) &time));

  tmptr = localtime ((time_t *) &time);
  printf ("localtime returns: ");
  printtm (tmptr);
  printf ("\n");
  printf ("mktime: %d\n\n", (int) mktime (tmptr));

  tmptr->tm_sec -= 20;
  tmptr->tm_min -= 20;
  tmptr->tm_hour -= 20;
  tmptr->tm_mday -= 20;
  tmptr->tm_mon -= 20;
  tmptr->tm_year -= 20;
  tmptr->tm_gmtoff -= 20000;	/* This has no effect! */
  tmptr->tm_zone = NULL;	/* Nor does this! */
  tmptr->tm_isdst = -1;

  printf ("changed ranges: ");
  printtm (tmptr);
  printf ("\n\n");

  result_time = mktime (tmptr);
  printf ("\nmktime: %d\n", result_time);
}
#endif /* DEBUG */
