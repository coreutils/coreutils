/* Copyright (C) 1995, 1997, 2000 Free Software Foundation, Inc.

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

/* Written by Jim Meyering <meyering@na-net.ornl.gov>.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

/* Copy LEN bytes starting at SRCADDR to DESTADDR.  Result undefined
   if the source overlaps with the destination.
   Return DESTADDR. */

char *
memcpy (char *destaddr, const char *srcaddr, int len)
{
  char *dest = destaddr;

  while (len-- > 0)
    *destaddr++ = *srcaddr++;
  return dest;
}
