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

/* Disable the definition of chown to rpl_chown (from config.h) in this
   file.  Otherwise, we'd get conflicting prototypes for rpl_chown on
   most systems.  */
#undef chown

#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

/* FIXME: describe.  */

int
rpl_chown (const char *file, uid_t uid, gid_t gid)
{
  if (gid == (gid_t) -1 || uid == (uid_t) -1)
    {
      struct stat file_stats;

      /* Stat file to get id(s) that should remain unchanged.  */
      if (stat (file, &file_stats))
	return 1;

      if (gid == (gid_t) -1)
	gid = file_stats.st_gid;

      if (uid == (uid_t) -1)
	uid = file_stats.st_uid;
    }

  return chown (file, uid, gid);
}
