/* provide consistent interface to chown for systems that don't interpret
   an ID of -1 as meaning `don't change the corresponding ID'.
   Copyright (C) 1997 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#include <config.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

/* FIXME.  */

int
chown (file, gid, uid)
     const char *file;
     gid_t git;
     uid_t uit;
{
  if (gid == (gid_t) -1 || uid == (uid_t) -1)
    {
      /* Stat file to get id(s) that will remain unchanged.  */
      FIXME
    }

#undef chown

  return chown (file, gid, uid);
}
