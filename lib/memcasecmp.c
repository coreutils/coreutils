/* Case-insensitive buffer comparator.
   Copyright (C) 1996 Free Software Foundation, Inc.

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

/* Jim Meyering (meyering@na-net.ornl.gov) */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <ctype.h>

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
#define ISASCII(c) 1
#else
#define ISASCII(c) isascii(c)
#endif
#define ISUPPER(c) (ISASCII (c) && isupper (c))

#if _LIBC || STDC_HEADERS
# define TOLOWER(c) tolower (c)
#else
# define TOLOWER(c) (ISUPPER (c) ? tolower (c) : (c))
#endif

#include "memcasecmp.h"

/* Like memcmp, but ignore differences in case.  */

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
      if (TOLOWER (u1) != TOLOWER (u2))
        return TOLOWER (u1) - TOLOWER (u2);
    }
  return 0;
}
