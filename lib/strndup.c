/* Copyright (C) 1996 Free Software Foundation, Inc.

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

#include <config.h>
#include <ansidecl.h>
#include "xstring.h"
#include "xmalloc.h"


/* Duplicate S, returning an identical malloc'd string.  */
char *
DEFUN(strndup, (s, n), const char *s AND size_t n)
{
  char *new = malloc(n + 1);

  if (new == NULL)
    return NULL;

  memcpy (new, s, n);
  new[n] = '\0';

  return new;
}
