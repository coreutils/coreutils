/* Copyright (C) 1991 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#ifndef strchr
#define strchr index
#endif
#endif

/* Return the length of the maximum inital segment of S
   which contains no characters from REJECT.  */
int
strcspn (s, reject)
     register char *s;
     register char *reject;
{
  register int count = 0;

  while (*s != '\0')
    if (strchr (reject, *s++) == 0)
      ++count;
    else
      return count;

  return count;
}
