/* linebuffer.c -- read arbitrarily long lines
   Copyright (C) 1986, 1991, 1998, 1999, 2001 Free Software Foundation, Inc.

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

/* Written by Richard Stallman. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include "linebuffer.h"
#include "unlocked-io.h"
#include "xalloc.h"

void free ();

/* Initialize linebuffer LINEBUFFER for use. */

void
initbuffer (struct linebuffer *linebuffer)
{
  linebuffer->length = 0;
  linebuffer->size = 200;
  linebuffer->buffer = xmalloc (linebuffer->size);
}

/* Read an arbitrarily long line of text from STREAM into LINEBUFFER.
   Keep the newline; append a newline if it's the last line of a file
   that ends in a non-newline character.  Do not null terminate.
   Therefore the stream can contain NUL bytes, and the length
   (including the newline) is returned in linebuffer->length.
   Return NULL upon error, or when STREAM is empty.
   Otherwise, return LINEBUFFER.  */
struct linebuffer *
readline (struct linebuffer *linebuffer, FILE *stream)
{
  int c;
  char *buffer = linebuffer->buffer;
  char *p = linebuffer->buffer;
  char *end = buffer + linebuffer->size; /* Sentinel. */

  if (feof (stream) || ferror (stream))
    return NULL;

  do
    {
      c = getc (stream);
      if (c == EOF)
	{
	  if (p == buffer)
	    return NULL;
	  if (p[-1] == '\n')
	    break;
	  c = '\n';
	}
      if (p == end)
	{
	  linebuffer->size *= 2;
	  buffer = xrealloc (buffer, linebuffer->size);
	  p = p - linebuffer->buffer + buffer;
	  linebuffer->buffer = buffer;
	  end = buffer + linebuffer->size;
	}
      *p++ = c;
    }
  while (c != '\n');

  linebuffer->length = p - buffer;
  return linebuffer;
}

/* Free linebuffer LINEBUFFER and its data, all allocated with malloc. */

void
freebuffer (struct linebuffer *linebuffer)
{
  free (linebuffer->buffer);
  free (linebuffer);
}
