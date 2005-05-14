/* utimecmp.h -- compare file time stamps

   Copyright (C) 2004 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Paul Eggert.  */

#ifndef UTIMECMP_H
#define UTIMECMP_H 1

#include <sys/types.h>
#include <sys/stat.h>

/* Options for utimecmp.  */
enum
{
  /* Before comparing, truncate the source time stamp to the
     resolution of the destination file system and to the resolution
     of utimens.  */
  UTIMECMP_TRUNCATE_SOURCE = 1
};

int utimecmp (char const *, struct stat const *, struct stat const *, int);

#endif
