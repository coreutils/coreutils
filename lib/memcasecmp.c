/* Case-insensitive buffer comparator.
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.

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

/* Jim Meyering (meyering@na-net.ornl.gov) */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <ctype.h>

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
# define IN_CTYPE_DOMAIN(c) 1
#else
# define IN_CTYPE_DOMAIN(c) isascii(c)
#endif
#define ISLOWER(c) (IN_CTYPE_DOMAIN (c) && islower (c))

#if _LIBC || STDC_HEADERS
# define TOUPPER(c) toupper (c)
#else
# define TOUPPER(c) (ISLOWER (c) ? toupper (c) : (c))
#endif

#include "memcasecmp.h"

/* Like memcmp, but ignore differences in case.
   Convert to upper case (not lower) before comparing so that
   join -i works with sort -f.  */

int
memcasecmp (vs1, vs2, n)
     const void *vs1;
     const void *vs2;
     size_t n;
{
  unsigned int i;
  unsigned char *s1 = (unsigned char *) vs1;
  unsigned char *s2 = (unsigned char *) vs2;
  for (i = 0; i < n; i++)
    {
      unsigned char u1 = *s1++;
      unsigned char u2 = *s2++;
      if (TOUPPER (u1) != TOUPPER (u2))
        return TOUPPER (u1) - TOUPPER (u2);
    }
  return 0;
}
