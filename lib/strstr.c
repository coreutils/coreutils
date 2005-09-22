/* Searching in a string.
   Copyright (C) 2005 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2005.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "strstr.h"

#include <stddef.h>  /* for NULL */

#if HAVE_MBRTOWC
# include "mbuiter.h"
#endif

/* Find the first occurrence of NEEDLE in HAYSTACK.  */
char *
strstr (const char *haystack, const char *needle)
{
  /* Be careful not to look at the entire extent of haystack or needle
     until needed.  This is useful because of these two cases:
       - haystack may be very long, and a match of needle found early,
       - needle may be very long, and not even a short initial segment of
         needle may be found in haystack.  */
#if HAVE_MBRTOWC
  if (MB_CUR_MAX > 1)
    {
      mbui_iterator_t iter_needle;

      mbui_init (iter_needle, needle);
      if (mbui_avail (iter_needle))
	{
	  mbui_iterator_t iter_haystack;

	  mbui_init (iter_haystack, haystack);
	  for (;; mbui_advance (iter_haystack))
	    {
	      if (!mbui_avail (iter_haystack))
		/* No match.  */
		return NULL;

	      if (mb_equal (mbui_cur (iter_haystack), mbui_cur (iter_needle)))
		/* The first character matches.  */
		{
		  mbui_iterator_t rhaystack;
		  mbui_iterator_t rneedle;

		  memcpy (&rhaystack, &iter_haystack, sizeof (mbui_iterator_t));
		  mbui_advance (rhaystack);

		  mbui_init (rneedle, needle);
		  if (!mbui_avail (rneedle))
		    abort ();
		  mbui_advance (rneedle);

		  for (;; mbui_advance (rhaystack), mbui_advance (rneedle))
		    {
		      if (!mbui_avail (rneedle))
			/* Found a match.  */
			return (char *) mbui_cur_ptr (iter_haystack);
		      if (!mbui_avail (rhaystack))
			/* No match.  */
			return NULL;
		      if (!mb_equal (mbui_cur (rhaystack), mbui_cur (rneedle)))
			/* Nothing in this round.  */
			break;
		    }
		}
	    }
	}
      else
	return (char *) haystack;
    }
  else
#endif
    {
      if (*needle != '\0')
	{
	  /* Speed up the following searches of needle by caching its first
	     character.  */
	  char b = *needle++;

	  for (;; haystack++)
	    {
	      if (*haystack == '\0')
		/* No match.  */
		return NULL;
	      if (*haystack == b)
		/* The first character matches.  */
		{
		  const char *rhaystack = haystack + 1;
		  const char *rneedle = needle;

		  for (;; rhaystack++, rneedle++)
		    {
		      if (*rneedle == '\0')
			/* Found a match.  */
			return (char *) haystack;
		      if (*rhaystack == '\0')
			/* No match.  */
			return NULL;
		      if (*rhaystack != *rneedle)
			/* Nothing in this round.  */
			break;
		    }
		}
	    }
	}
      else
	return (char *) haystack;
    }
}
