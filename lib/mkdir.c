/* mkdir.c -- BSD compatible make directory function for System V
   Copyright (C) 1988, 1990 Free Software Foundation, Inc.

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
#include <errno.h>
#ifndef errno
extern int errno;
#endif

#ifdef STAT_MACROS_BROKEN
#undef S_ISDIR
#endif

#if !defined(S_ISDIR) && defined(S_IFDIR)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

/* mkdir adapted from GNU tar.  */

/* Make directory DPATH, with permission mode DMODE.

   Written by Robert Rother, Mariah Corporation, August 1985
   (sdcsvax!rmr or rmr@uscd).  If you want it, it's yours.

   Severely hacked over by John Gilmore to make a 4.2BSD compatible
   subroutine.	11Mar86; hoptoad!gnu

   Modified by rmtodd@uokmax 6-28-87 -- when making an already existing dir,
   subroutine didn't return EEXIST.  It does now.  */

int
mkdir (dpath, dmode)
     char *dpath;
     int dmode;
{
  int cpid, status;
  struct stat statbuf;

  if (stat (dpath, &statbuf) == 0)
    {
      errno = EEXIST;		/* stat worked, so it already exists.  */
      return -1;
    }

  /* If stat fails for a reason other than non-existence, return error.  */
  if (errno != ENOENT)
    return -1;

  cpid = fork ();
  switch (cpid)
    {
    case -1:			/* Cannot fork.  */
      return -1;		/* errno is already set.  */

    case 0:			/* Child process.  */
      /* Cheap hack to set mode of new directory.  Since this child
	 process is going away anyway, we zap its umask.
	 This won't suffice to set SUID, SGID, etc. on this
	 directory, so the parent process calls chmod afterward.  */
      status = umask (0);	/* Get current umask.  */
      umask (status | (0777 & ~dmode));	/* Set for mkdir.  */
      execl ("/bin/mkdir", "mkdir", dpath, (char *) 0);
      _exit (1);

    default:			/* Parent process.  */
      /* Wait for kid to finish.  */
      while (wait (&status) != cpid)
	/* Do nothing.  */ ;

      if (status & 0xFFFF)
	{
	  /* /bin/mkdir failed.  */
	  errno = EIO;
	  return -1;
	}
      return chmod (dpath, dmode);
    }
}
