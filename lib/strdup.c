/* Copyright (C) 1991, 1996, 1997, 1998, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if defined _LIBC || defined  STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else
char *malloc ();
char *memcpy ();
#endif

#undef __strdup
#undef strdup

#ifndef weak_alias
# define __strdup strdup
#endif

/* Duplicate S, returning an identical malloc'd string.  */
char *
__strdup (const char *s)
{
  size_t len = strlen (s) + 1;
  void *new = malloc (len);

  if (new == NULL)
    return NULL;

  return (char *) memcpy (new, s, len);
}
#ifdef libc_hidden_def
libc_hidden_def (__strdup)
#endif
#ifdef weak_alias
weak_alias (__strdup, strdup)
#endif
