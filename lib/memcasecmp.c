/* Case-insensitive buffer comparator.
   Copyright (C) 1996, 1997, 2000 Free Software Foundation, Inc.

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

/* Written by Jim Meyering.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <ctype.h>

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
# define IN_CTYPE_DOMAIN(Char) 1
#else
# define IN_CTYPE_DOMAIN(Char) isascii(Char)
#endif
#define ISLOWER(Char) (IN_CTYPE_DOMAIN (Char) && islower (Char))

#if _LIBC || STDC_HEADERS
# define TOUPPER(Char) toupper (Char)
#else
# define TOUPPER(Char) (ISLOWER (Char) ? toupper (Char) : (Char))
#endif

#include "memcasecmp.h"

/* Like memcmp, but ignore differences in case.
   Convert to upper case (not lower) before comparing so that
   join -i works with sort -f.  */

int
memcasecmp (const void *vs1, const void *vs2, size_t n)
{
  unsigned int i;
  unsigned char const *s1 = (unsigned char const *) vs1;
  unsigned char const *s2 = (unsigned char const *) vs2;
  for (i = 0; i < n; i++)
    {
      unsigned char u1 = *s1++;
      unsigned char u2 = *s2++;
      if (TOUPPER (u1) != TOUPPER (u2))
        return TOUPPER (u1) - TOUPPER (u2);
    }
  return 0;
}
