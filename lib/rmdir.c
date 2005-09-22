/* BSD compatible remove directory function for System V

   Copyright (C) 1988, 1990, 1999, 2003, 2004, 2005 Free Software
   Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "stat-macros.h"

/* rmdir adapted from GNU tar.  */

/* Remove directory DIR.
   Return 0 if successful, -1 if not.  */

int
rmdir (char const *dir)
{
  pid_t cpid;
  int status;
  struct stat statbuf;

  if (stat (dir, &statbuf) != 0)
    return -1;			/* errno already set */

  if (!S_ISDIR (statbuf.st_mode))
    {
      errno = ENOTDIR;
      return -1;
    }

  cpid = fork ();
  switch (cpid)
    {
    case -1:			/* cannot fork */
      return -1;		/* errno already set */

    case 0:			/* child process */
      execl ("/bin/rmdir", "rmdir", dir, (char *) 0);
      _exit (1);

    default:			/* parent process */

      /* Wait for kid to finish.  */

      while (wait (&status) != cpid)
	/* Do nothing.  */ ;

      if (status)
	{

	  /* /bin/rmdir failed.  */

	  errno = EIO;
	  return -1;
	}
      return 0;
    }
}
