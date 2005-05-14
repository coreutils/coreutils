/* linebuffer.c -- read arbitrarily long lines

   Copyright (C) 1986, 1991, 1998, 1999, 2001, 2003, 2004 Free
   Software Foundation, Inc.

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

/* Written by Richard Stallman. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "linebuffer.h"
#include "xalloc.h"

#if USE_UNLOCKED_IO
# include "unlocked-io.h"
#endif

/* Initialize linebuffer LINEBUFFER for use. */

void
initbuffer (struct linebuffer *linebuffer)
{
  memset (linebuffer, 0, sizeof *linebuffer);
}

/* Read an arbitrarily long line of text from STREAM into LINEBUFFER.
   Keep the newline; append a newline if it's the last line of a file
   that ends in a non-newline character.  Do not null terminate.
   Therefore the stream can contain NUL bytes, and the length
   (including the newline) is returned in linebuffer->length.
   Return NULL when stream is empty.  Return NULL and set errno upon
   error; callers can distinguish this case from the empty case by
   invoking ferror (stream).
   Otherwise, return LINEBUFFER.  */
struct linebuffer *
readlinebuffer (struct linebuffer *linebuffer, FILE *stream)
{
  int c;
  char *buffer = linebuffer->buffer;
  char *p = linebuffer->buffer;
  char *end = buffer + linebuffer->size; /* Sentinel. */

  if (feof (stream))
    return NULL;

  do
    {
      c = getc (stream);
      if (c == EOF)
	{
	  if (p == buffer || ferror (stream))
	    return NULL;
	  if (p[-1] == '\n')
	    break;
	  c = '\n';
	}
      if (p == end)
	{
	  size_t oldsize = linebuffer->size;
	  buffer = x2realloc (buffer, &linebuffer->size);
	  p = buffer + oldsize;
	  linebuffer->buffer = buffer;
	  end = buffer + linebuffer->size;
	}
      *p++ = c;
    }
  while (c != '\n');

  linebuffer->length = p - buffer;
  return linebuffer;
}

/* Free the buffer that was allocated for linebuffer LINEBUFFER.  */

void
freebuffer (struct linebuffer *linebuffer)
{
  free (linebuffer->buffer);
}
