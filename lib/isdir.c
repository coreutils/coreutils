/* isdir.c -- determine whether a directory exists
   Copyright (C) 1990 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <sys/stat.h>

#include "safe-stat.h"

#ifdef STAT_MACROS_BROKEN
#undef S_ISDIR
#endif /* STAT_MACROS_BROKEN.  */

#if !defined(S_ISDIR) && defined(S_IFDIR)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

/* If PATH is an existing directory or symbolic link to a directory,
   return nonzero, else 0.  */

int
isdir (path)
     char *path;
{
  struct stat stats;

  return safe_stat (path, &stats) == 0 && S_ISDIR (stats.st_mode);
}
