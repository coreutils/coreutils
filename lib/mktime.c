/* Copyright (C) 1993, 1994 Free Software Foundation, Inc.
   Contributed by Noel Cragg (noel@cs.oberlin.edu), with fixes by
   Michael E. Calwas (calwas@ttd.teradyne.com) and
   Wade Hampton (tasi029@tmn.com).


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

/* Define this to have a standalone program to test this implementation of
   mktime.  */
/* #define DEBUG */

#ifdef HAVE_CONFIG_H
#include <config.h>
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


#ifdef DEBUG

#include <stdio.h>
#include <ctype.h>

int debugging_enabled = 0;

/* Print the values in a `struct tm'. */
static void
printtm (it)
     struct tm *it;
{
  printf ("%02d/%02d/%04d %02d:%02d:%02d (%s) yday:%03d dst:%d gmtoffset:%ld",
	  it->tm_mon + 1,
	  it->tm_mday,
	  it->tm_year + 1900,
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
      

/* MKTIME converts the values in a struct tm to a time_t.  The values
   in tm_wday and tm_yday are ignored; other values can be put outside
   of legal ranges since they will be normalized.  This routine takes
   care of that normalization. */

void
do_normalization (tmptr)
     struct tm *tmptr;
{

#define normalize(foo,x,y,bar); \
  while (tmptr->foo < x) \
    { \
      tmptr->bar--; \
      tmptr->foo = (y - (x - tmptr->foo) + 1); \
    } \
  while (tmptr->foo > y) \
    { \
      tmptr->foo = (x + (tmptr->foo - y) - 1); \
      tmptr->bar++; \
    }
  
  normalize (tm_sec, 0, 59, tm_min);
  normalize (tm_min, 0, 59, tm_hour);
  normalize (tm_hour, 0, 23, tm_mday);
  
  /* Do the month first, so day range can be found. */
  normalize (tm_mon, 0, 11, tm_year);

  /* Since the day range modifies the month, we should be careful how
     we reference the array of month lengths -- it is possible that
     the month will go negative, hence the modulo...

     Also, tm_year is the year - 1900, so we have to 1900 to have it
     work correctly. */

  normalize (tm_mday, 1,
	     __mon_lengths[__isleap (tmptr->tm_year + 1900)]
                          [((tmptr->tm_mon < 0)
			    ? (12 + (tmptr->tm_mon % 12))
			    : (tmptr->tm_mon % 12)) ],
	     tm_mon);

  /* Do the month again, because the day may have pushed it out of range. */
  normalize (tm_mon, 0, 11, tm_year);

  /* Do the day again, because the month may have changed the range. */
  normalize (tm_mday, 1,
	     __mon_lengths[__isleap (tmptr->tm_year + 1900)]
	                  [((tmptr->tm_mon < 0)
			    ? (12 + (tmptr->tm_mon % 12))
			    : (tmptr->tm_mon % 12)) ],
	     tm_mon);
  
#ifdef DEBUG
  if (debugging_enabled)
    {
      printf ("   After normalizing:\n     ");
      printtm (tmptr);
      putchar ('\n');
    }
#endif

}


/* Here's where the work gets done. */

#define BAD_STRUCT_TM ((time_t) -1)

time_t
_mktime_internal (timeptr, producer)
     struct tm *timeptr;
     struct tm *(*producer) __P ((const time_t *));
{
  struct tm our_tm;		/* our working space */
  struct tm *me = &our_tm;	/* a pointer to the above */
  time_t result;		/* the value we return */

  *me = *timeptr;		/* copy the struct tm that was passed
				   in by the caller */


  /***************************/
  /* Normalize the structure */
  /***************************/

  /* This routine assumes that the value of TM_ISDST is -1, 0, or 1.
     If the user didn't pass it in that way, fix it. */

  if (me->tm_isdst > 0)
    me->tm_isdst = 1;
  else if (me->tm_isdst < 0)
    me->tm_isdst = -1;

  do_normalization (me);

  /* Get out of here if it's not possible to represent this struct.
     If any of the values in the normalized struct tm are negative,
     our algorithms won't work.  Luckily, we only need to check the
     year at this point; normalization guarantees that all values will
     be in correct ranges EXCEPT the year. */

  if (me->tm_year < 0)
    return BAD_STRUCT_TM;

  /*************************************************/
  /* Find the appropriate time_t for the structure */
  /*************************************************/

  /* Modified b-search -- make intelligent guesses as to where the
     time might lie along the timeline, assuming that our target time
     lies a linear distance (w/o considering time jumps of a
     particular region).

     Assume that time does not fluctuate at all along the timeline --
     e.g., assume that a day will always take 86400 seconds, etc. --
     and come up with a hypothetical value for the time_t
     representation of the struct tm TARGET, in relation to the guess
     variable -- it should be pretty close!

     After testing this, the maximum number of iterations that I had
     on any number that I tried was 3!  Not bad.

     The reason this is not a subroutine is that we will modify some
     fields in the struct tm (yday and mday).  I've never felt good
     about side-effects when writing structured code... */

  {
    struct tm *guess_tm;
    time_t guess = 0;
    time_t distance = 0;
    time_t last_distance = 0;

    times_through_search = 0;

    do
      {
	guess += distance;

	times_through_search++;     
      
	guess_tm = (*producer) (&guess);
      
#ifdef DEBUG
	if (debugging_enabled)
	  {
	    printf ("   Guessing time_t == %d\n     ", (int) guess);
	    printtm (guess_tm);
	    putchar ('\n');
	  }
#endif
      
	/* How far is our guess from the desired struct tm? */
	distance = dist_tm (me, guess_tm);
      
	/* Handle periods of time where a period of time is skipped.
	   For example, 2:15 3 April 1994 does not exist, because DST
	   is in effect.  The distance function will alternately
	   return values of 3600 and -3600, because it doesn't know
	   that the requested time doesn't exist.  In these situations
	   (even if the skip is not exactly an hour) the distances
	   returned will be the same, but alternating in sign.  We
	   want the later time, so check to see that the distance is
	   oscillating and we've chosen the correct of the two
	   possibilities.

	   Useful: 3 Apr 94 765356300, 30 Oct 94 783496000 */

	if ((distance == -last_distance) && (distance < last_distance))
	  {
	    /* If the caller specified that the DST flag was off, it's
               not possible to represent this time. */
	    if (me->tm_isdst == 0)
	      {
#ifdef DEBUG
	    printf ("   Distance is oscillating -- dst flag nixes struct!\n");
#endif
		return BAD_STRUCT_TM;
	      }

#ifdef DEBUG
	    printf ("   Distance is oscillating -- chose the later time.\n");
#endif
	    distance = 0;
	  }

	if ((distance == 0) && (me->tm_isdst != -1)
	    && (me->tm_isdst != guess_tm->tm_isdst))
	  {
	    /* If we're in this code, we've got the right time but the
               wrong daylight savings flag.  We need to move away from
               the time that we have and approach the other time from
               the other direction.  That is, if I've requested the
               non-DST version of a time and I get the DST version
               instead, I want to put us forward in time and search
               backwards to get the other time.  I checked all of the
               configuration files for the tz package -- no entry
               saves more than two hours, so I think we'll be safe by
               moving 24 hours in one direction.  IF THE AMOUNT OF
               TIME SAVED IN THE CONFIGURATION FILES CHANGES, THIS
               VALUE MAY NEED TO BE ADJUSTED.  Luckily, we can never
               have more than one level of overlaps, or this would
               never work. */

#define SKIP_VALUE 86400

	    if (guess_tm->tm_isdst == 0)
	      /* we got the later one, but want the earlier one */
	      distance = -SKIP_VALUE;
	    else
	      distance = SKIP_VALUE;
	    
#ifdef DEBUG
	    printf ("   Got the right time, wrong DST value -- adjusting\n");
#endif
	  }

	last_distance = distance;

      } while (distance != 0);

    /* Check to see that the dst flag matches */

    if (me->tm_isdst != -1)
      {
	if (me->tm_isdst != guess_tm->tm_isdst)
	  {
#ifdef DEBUG
	    printf ("   DST flag doesn't match!  FIXME?\n");
#endif
	    return BAD_STRUCT_TM;
	  }
      }

    result = guess;		/* Success! */

    /* On successful completion, the values of tm_wday and tm_yday
       have to be set appropriately. */
    
    /* me->tm_yday = guess_tm->tm_yday; 
       me->tm_mday = guess_tm->tm_mday; */

    *me = *guess_tm;
  }

  /* Update the caller's version of the structure */

  *timeptr = *me;

  return result;
}

time_t
#ifdef DEBUG			/* make it work even if the system's
				   libc has it's own mktime routine */
my_mktime (timeptr)
#else
mktime (timeptr)
#endif
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

      for (q = 10000000; q < 1000000000; q += 599)
	{
	  struct tm *tm = localtime ((time_t *) &q);
	  if ((q % 10000) == 0) { printf ("%ld\n", q); fflush (stdout); }
	  if (q != my_mktime (tm))
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
  
  tmptr = localtime ((time_t *) &time);
  printf ("Localtime tells us that a time_t of %d represents\n     ", time);
  printtm (tmptr);
  putchar ('\n');

  printf ("   Given localtime's return val, mktime returns %d which is\n     ",
	  (int) my_mktime (tmptr));
  printtm (tmptr);
  putchar ('\n');

#if 0
  tmptr->tm_sec -= 20;
  tmptr->tm_min -= 20;
  tmptr->tm_hour -= 20;
  tmptr->tm_mday -= 20;
  tmptr->tm_mon -= 20;
  tmptr->tm_year -= 20;
  tmptr->tm_gmtoff -= 20000;	/* This has no effect! */
  tmptr->tm_zone = NULL;	/* Nor does this! */
  tmptr->tm_isdst = -1;
#endif
  
  tmptr->tm_hour += 1;
  tmptr->tm_isdst = -1;

  printf ("\n\nchanged ranges: ");
  printtm (tmptr);
  putchar ('\n');

  result_time = my_mktime (tmptr);
  printf ("\nmktime: %d\n", result_time);

  tmptr->tm_isdst = 0;

  printf ("\n\nchanged ranges: ");
  printtm (tmptr);
  putchar ('\n');

  result_time = my_mktime (tmptr);
  printf ("\nmktime: %d\n", result_time);
}
#endif /* DEBUG */


/*
Local Variables:
compile-command: "gcc -g mktime.c -o mktime -DDEBUG"
End:
*/
