/* getndelim2 - Read a line from a stream, stopping at one of 2 delimiters,
   with bounded memory allocation.

   Copyright (C) 1993, 1996, 1997, 1998, 2000, 2003, 2004 Free Software
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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Originally written by Jan Brittenson, bson@gnu.ai.mit.edu.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "getndelim2.h"

#include <stdlib.h>
#include <stddef.h>

#if USE_UNLOCKED_IO
# include "unlocked-io.h"
#endif

#include <limits.h>
#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#if HAVE_STDINT_H
# include <stdint.h>
#endif
#ifndef PTRDIFF_MAX
# define PTRDIFF_MAX ((ptrdiff_t) (SIZE_MAX / 2))
#endif
#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) -1)
#endif
#ifndef SSIZE_MAX
# define SSIZE_MAX ((ssize_t) (SIZE_MAX / 2))
#endif

/* The maximum value that getndelim2 can return without suffering from
   overflow problems, either internally (because of pointer
   subtraction overflow) or due to the API (because of ssize_t).  */
#define GETNDELIM2_MAXIMUM (PTRDIFF_MAX < SSIZE_MAX ? PTRDIFF_MAX : SSIZE_MAX)

/* Try to add at least this many bytes when extending the buffer.
   MIN_CHUNK must be no greater than GETNDELIM2_MAXIMUM.  */
#define MIN_CHUNK 64

ssize_t
getndelim2 (char **lineptr, size_t *linesize, size_t offset, size_t nmax,
            int delim1, int delim2, FILE *stream)
{
  size_t nbytes_avail;		/* Allocated but unused bytes in *LINEPTR.  */
  char *read_pos;		/* Where we're reading into *LINEPTR. */
  ssize_t bytes_stored = -1;
  char *ptr = *lineptr;
  size_t size = *linesize;

  if (!ptr)
    {
      size = nmax < MIN_CHUNK ? nmax : MIN_CHUNK;
      ptr = malloc (size);
      if (!ptr)
	return -1;
    }

  if (size < offset)
    goto done;

  nbytes_avail = size - offset;
  read_pos = ptr + offset;

  if (nbytes_avail == 0 && nmax <= size)
    goto done;

  for (;;)
    {
      /* Here always ptr + size == read_pos + nbytes_avail.  */

      int c;

      /* We always want at least one byte left in the buffer, since we
	 always (unless we get an error while reading the first byte)
	 NUL-terminate the line buffer.  */

      if (nbytes_avail < 2 && size < nmax)
	{
	  size_t newsize = size < MIN_CHUNK ? size + MIN_CHUNK : 2 * size;
	  char *newptr;

	  if (! (size < newsize && newsize <= nmax))
	    newsize = nmax;

	  if (GETNDELIM2_MAXIMUM < newsize - offset)
	    {
	      size_t newsizemax = offset + GETNDELIM2_MAXIMUM + 1;
	      if (size == newsizemax)
		goto done;
	      newsize = newsizemax;
	    }

	  nbytes_avail = newsize - (read_pos - ptr);
	  newptr = realloc (ptr, newsize);
	  if (!newptr)
	    goto done;
	  ptr = newptr;
	  size = newsize;
	  read_pos = size - nbytes_avail + ptr;
	}

      c = getc (stream);
      if (c == EOF)
	{
	  /* Return partial line, if any.  */
	  if (read_pos == ptr)
	    goto done;
	  else
	    break;
	}

      if (nbytes_avail >= 2)
	{
	  *read_pos++ = c;
	  nbytes_avail--;
	}

      if (c == delim1 || c == delim2)
	/* Return the line.  */
	break;
    }

  /* Done - NUL terminate and return the number of bytes read.
     At this point we know that nbytes_avail >= 1.  */
  *read_pos = '\0';

  bytes_stored = read_pos - (ptr + offset);

 done:
  *lineptr = ptr;
  *linesize = size;
  return bytes_stored;
}
