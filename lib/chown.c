/* provide consistent interface to chown for systems that don't interpret
   an ID of -1 as meaning `don't change the corresponding ID'.
   Copyright (C) 1997, 2004, 2005 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Disable the definition of chown to rpl_chown (from config.h) in this
   file.  Otherwise, we'd get conflicting prototypes for rpl_chown on
   most systems.  */
#undef chown

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* Provide a more-closely POSIX-conforming version of chown on
   systems with one or both of the following problems:
   - chown doesn't treat an ID of -1 as meaning
   `don't change the corresponding ID'.
   - chown doesn't dereference symlinks.  */

int
rpl_chown (const char *file, uid_t uid, gid_t gid)
{
#if CHOWN_FAILS_TO_HONOR_ID_OF_NEGATIVE_ONE
  if (gid == (gid_t) -1 || uid == (uid_t) -1)
    {
      struct stat file_stats;

      /* Stat file to get id(s) that should remain unchanged.  */
      if (stat (file, &file_stats))
	return -1;

      if (gid == (gid_t) -1)
	gid = file_stats.st_gid;

      if (uid == (uid_t) -1)
	uid = file_stats.st_uid;
    }
#endif

#if CHOWN_MODIFIES_SYMLINK
  {
    /* Handle the case in which the system-supplied chown function
       does *not* follow symlinks.  Instead, it changes permissions
       on the symlink itself.  To work around that, we open the
       file (but this can fail due to lack of read or write permission) and
       use fchown on the resulting descriptor.  */
    int fd = open (file, O_RDONLY | O_NONBLOCK | O_NOCTTY);
    if (fd < 0
	&& (fd = open (file, O_WRONLY | O_NONBLOCK | O_NOCTTY)) < 0)
      return -1;
    if (fchown (fd, uid, gid))
      {
	int saved_errno = errno;
	close (fd);
	errno = saved_errno;
	return -1;
      }
    return close (fd);
  }
#else
  return chown (file, uid, gid);
#endif
}
