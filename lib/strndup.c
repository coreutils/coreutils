/* Copyright (C) 1996 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <string.h>
# include <stdlib.h>
#else
char *malloc ();
#endif

/* Duplicate S, returning an identical malloc'd string.  */
char *
strndup (s, n)
     const char *s;
     size_t n;
{
  char *new = malloc (n + 1);

  if (new == NULL)
    return NULL;

  memcpy (new, s, n);
  new[n] = '\0';

  return new;
}
