/* getndelim2 - Read a line from a stream, stopping at one of 2 delimiters,
   with bounded memory allocation.

   Copyright (C) 2003 Free Software Foundation, Inc.

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

#ifndef GETNDELIM2_H
#define GETNDELIM2_H 1

#include <stddef.h>
#include <stdio.h>

/* Get ssize_t.  */
#include <sys/types.h>

/* Read up to (and including) a delimiter DELIM1 from STREAM into *LINEPTR
   + OFFSET (and NUL-terminate it).  If DELIM2 is non-zero, then read up
   and including the first occurrence of DELIM1 or DELIM2.  *LINEPTR is
   a pointer returned from malloc (or NULL), pointing to *LINESIZE bytes of
   space.  It is realloc'd as necessary.  Reallocation is limited to
   NMAX bytes; if the line is longer than that, the extra bytes are read but
   thrown away.
   Return the number of bytes read and stored at *LINEPTR + OFFSET (not
   including the NUL terminator), or -1 on error or EOF.  */
extern ssize_t getndelim2 (char **lineptr, size_t *linesize, size_t nmax,
			   FILE *stream, int delim1, int delim2,
			   size_t offset);

#endif /* GETNDELIM2_H */
