/* Copyright (C) 1995, 1997, 2000, 2003 Free Software Foundation, Inc.

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

#include <stddef.h>

/* Copy LEN bytes starting at SRCADDR to DESTADDR.  Result undefined
   if the source overlaps with the destination.
   Return DESTADDR. */

void *
memcpy (void *destaddr, void const *srcaddr, size_t len)
{
  char *dest = destaddr;
  char const *src = srcaddr;

  while (len-- > 0)
    *dest++ = *src++;
  return destaddr;
}
