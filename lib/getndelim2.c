/* getndelim2 - Read a line from a stream, stopping at one of 2 delimiters,
   with bounded memory allocation.

   Copyright (C) 1993, 1996, 1997, 1998, 2000, 2003 Free Software
   Foundation, Inc.

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

/* Originally written by Jan Brittenson, bson@gnu.ai.mit.edu.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "getndelim2.h"

#if STDC_HEADERS
# include <stdlib.h>
#else
char *malloc (), *realloc ();
#endif

#include "unlocked-io.h"

/* Always add at least this many bytes when extending the buffer.  */
#define MIN_CHUNK 64

ssize_t
getndelim2 (char **lineptr, size_t *linesize, size_t nmax,
	    FILE *stream, int delim1, int delim2, size_t offset)
{
  size_t nbytes_avail;		/* Allocated but unused chars in *LINEPTR.  */
  char *read_pos;		/* Where we're reading into *LINEPTR. */

  if (!lineptr || !linesize || !nmax || !stream)
    return -1;

  if (!*lineptr)
    {
      size_t newlinesize = MIN_CHUNK;

      if (newlinesize > nmax)
	newlinesize = nmax;

      *linesize = newlinesize;
      *lineptr = malloc (*linesize);
      if (!*lineptr)
	return -1;
    }

  if (*linesize < offset)
    return -1;

  nbytes_avail = *linesize - offset;
  read_pos = *lineptr + offset;

  if (nbytes_avail == 0 && *linesize >= nmax)
    return -1;

  for (;;)
    {
      /* Here always *lineptr + *linesize == read_pos + nbytes_avail.  */

      register int c = getc (stream);

      /* We always want at least one char left in the buffer, since we
	 always (unless we get an error while reading the first char)
	 NUL-terminate the line buffer.  */

      if (nbytes_avail < 2 && *linesize < nmax)
	{
	  size_t newlinesize =
	    (*linesize > MIN_CHUNK ? 2 * *linesize : *linesize + MIN_CHUNK);

	  if (newlinesize > nmax)
	    newlinesize = nmax;

	  if (newlinesize > *linesize)
	    {
	      *linesize = newlinesize;
	      nbytes_avail = *linesize + *lineptr - read_pos;
	      *lineptr = realloc (*lineptr, *linesize);
	      if (!*lineptr)
		return -1;
	      read_pos = *linesize - nbytes_avail + *lineptr;
	    }
	}

      if (c == EOF || ferror (stream))
	{
	  /* Return partial line, if any.  */
	  if (read_pos == *lineptr)
	    return -1;
	  else
	    break;
	}

      if (nbytes_avail >= 2)
	{
	  *read_pos++ = c;
	  nbytes_avail--;
	}

      if (c == delim1 || (delim2 && c == delim2))
	/* Return the line.  */
	break;
    }

  /* Done - NUL terminate and return the number of chars read.
     At this point we know that nbytes_avail >= 1.  */
  *read_pos = '\0';

  return read_pos - (*lineptr + offset);
}
