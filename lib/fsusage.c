/* fsusage.c -- return space usage of mounted filesystems
   Copyright (C) 1991, 1992 Free Software Foundation, Inc.

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
#if defined (CONFIG_BROKETS)
/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because it found this file in $srcdir).  */
#include <config.h>
#else
#include "config.h"
#endif
#endif

#include <sys/types.h>
#include "fsusage.h"

int statfs ();

#if defined (STAT_STATFS3_OSF1)	/* DEC Alpha running OSF/1 */
#  include <sys/mount.h>
#endif

#if defined(STAT_STATFS2_BSIZE) && !defined(_IBMR2) /* 4.3BSD, SunOS 4, HP-UX, AIX PS/2.  */
#include <sys/vfs.h>
#endif

#ifdef STAT_STATFS2_FSIZE	/* 4.4BSD.  */
#include <sys/mount.h>
#endif

#ifdef STAT_STATFS2_FS_DATA	/* Ultrix.  */
#include <sys/param.h>
#include <sys/mount.h>
#endif

#ifdef STAT_READ		/* SVR2.  */
#include <sys/param.h>
#include <sys/filsys.h>
#include <fcntl.h>
#endif

#if defined(STAT_STATFS4) || (defined(_AIX) && defined(_IBMR2)) /* SVR3, Dynix, Irix, AIX RS6000.  */
#include <sys/statfs.h>
#endif

#if defined(_AIX) && defined(_I386) /* AIX PS/2.  */
#include <sys/stat.h>
#include <sys/dustat.h>
#endif

#ifdef STAT_STATVFS		/* SVR4.  */
#include <sys/statvfs.h>
int statvfs ();
#endif

/* Set B_NEW to the number of TOSIZE-byte blocks used by B FROMSIZE-byte
   blocks, rounding away from zero.  TOSIZE must be positive.  Return -1
   from invoking function if FROMSIZE is not positive.  */

#define ADJUST_BLOCKS(b_new, b, fromsize, tosize)			\
  do									\
    {									\
      if ((tosize) <= 0)						\
	abort ();							\
      if ((fromsize) <= 0)						\
	return -1;							\
									\
      if ((fromsize) == (tosize))	/* E.g., from 512 to 512.  */	\
	(b_new) = (b);							\
      else if ((fromsize) > (tosize))	/* E.g., from 2048 to 512.  */	\
	(b_new) = (b) * ((fromsize) / (tosize));			\
      else				/* E.g., from 256 to 512.  */	\
	(b_new) = ((b) + ((b) < 0 ? -1 : 1)) / ((tosize) / (fromsize));	\
    }									\
  while (0)

/* Fill in the fields of FSP with information about space usage for
   the filesystem on which PATH resides.
   DISK is the device on which PATH is mounted, for space-getting
   methods that need to know it.
   Return 0 if successful, -1 if not. */

int
get_fs_usage (path, disk, fsp)
     char *path, *disk;
     struct fs_usage *fsp;
{
#if defined (STAT_STATFS3_OSF1)
  struct statfs fsd;

  if (statfs (path, &fsd, sizeof (struct statfs)) != 0)
    return -1;
#define CONVERT_BLOCKS(bnew, b) ADJUST_BLOCKS ((bnew), (b), fsd.f_fsize, 512)
#endif /* STAT_STATFS3_OSF1 */

#ifdef STAT_STATFS2_FS_DATA	/* Ultrix.  */
  struct fs_data fsd;

  if (statfs (path, &fsd) != 1)
    return -1;
#define CONVERT_BLOCKS(bnew, b) ADJUST_BLOCKS ((bnew), (b), 1024, 512)
  CONVERT_BLOCKS (fsp->fsu_blocks, fsd.fd_req.btot);
  CONVERT_BLOCKS (fsp->fsu_bfree, fsd.fd_req.bfree);
  CONVERT_BLOCKS (fsp->fsu_bavail, fsd.fd_req.bfreen);
  fsp->fsu_files = fsd.fd_req.gtot;
  fsp->fsu_ffree = fsd.fd_req.gfree;
#endif

#ifdef STAT_READ		/* SVR2.  */
#ifndef SUPERBOFF
#define SUPERBOFF (SUPERB * 512)
#endif
  struct filsys fsd;
  int fd;

  fd = open (disk, O_RDONLY);
  if (fd < 0)
    return -1;
  lseek (fd, (long) SUPERBOFF, 0);
  if (read (fd, (char *) &fsd, sizeof fsd) != sizeof fsd)
    {
      close (fd);
      return -1;
    }
  close (fd);
#define CONVERT_BLOCKS(bnew, b) \
    ADJUST_BLOCKS ((bnew), (b), (fsd.s_type == Fs2b ? 1024 : 512), 512)
  CONVERT_BLOCKS (fsp->fsu_blocks, fsd.s_fsize);
  CONVERT_BLOCKS (fsp->fsu_bfree, fsd.s_tfree);
  CONVERT_BLOCKS (fsp->fsu_bavail, fsd.s_tfree);
  fsp->fsu_files = (fsd.s_isize - 2) * INOPB * (fsd.s_type == Fs2b ? 2 : 1);
  fsp->fsu_ffree = fsd.s_tinode;
#endif

#ifdef STAT_STATFS2_BSIZE	/* 4.3BSD, SunOS 4, HP-UX, AIX.  */
  struct statfs fsd;

  if (statfs (path, &fsd) < 0)
    return -1;
#define CONVERT_BLOCKS(bnew, b) ADJUST_BLOCKS ((bnew), (b), fsd.f_bsize, 512)
#endif

#ifdef STAT_STATFS2_FSIZE	/* 4.4BSD.  */
  struct statfs fsd;

  if (statfs (path, &fsd) < 0)
    return -1;
#define CONVERT_BLOCKS(bnew, b) ADJUST_BLOCKS ((bnew), (b), fsd.f_fsize, 512)
#endif

#ifdef STAT_STATFS4		/* SVR3, Dynix, Irix.  */
  struct statfs fsd;

  if (statfs (path, &fsd, sizeof fsd, 0) < 0)
    return -1;
  /* Empirically, the block counts on most SVR3 and SVR3-derived
     systems seem to always be in terms of 512-byte blocks,
     no matter what value f_bsize has.  */
#define CONVERT_BLOCKS(bnew, b)						\
  do									\
    {									\
      (bnew) = (b);							\
    }									\
  while (0)

#ifndef _SEQUENT_		/* _SEQUENT_ is DYNIX/ptx.  */
#ifndef DOLPHIN			/* DOLPHIN 3.8.alfa/7.18 has f_bavail */
#define f_bavail f_bfree
#endif
#endif
#endif

#ifdef STAT_STATVFS		/* SVR4.  */
  struct statvfs fsd;

  if (statvfs (path, &fsd) < 0)
    return -1;
  /* f_frsize isn't guaranteed to be supported.  */
#define CONVERT_BLOCKS(bnew, b) \
  ADJUST_BLOCKS ((bnew), (b), fsd.f_frsize ? fsd.f_frsize : fsd.f_bsize, 512)
#endif

#if !defined(STAT_STATFS2_FS_DATA) && !defined(STAT_READ) /* !Ultrix && !SVR2.  */
  CONVERT_BLOCKS (fsp->fsu_blocks, fsd.f_blocks);
  CONVERT_BLOCKS (fsp->fsu_bfree, fsd.f_bfree);
  CONVERT_BLOCKS (fsp->fsu_bavail, fsd.f_bavail);
  fsp->fsu_files = fsd.f_files;
  fsp->fsu_ffree = fsd.f_ffree;
#endif

  return 0;
}

#if defined(_AIX) && defined(_I386)
/* AIX PS/2 does not supply statfs.  */

int
statfs (path, fsb)
     char *path;
     struct statfs *fsb;
{
  struct stat stats;
  struct dustat fsd;

  if (stat (path, &stats))
    return -1;
  if (dustat (stats.st_dev, 0, &fsd, sizeof (fsd)))
    return -1;
  fsb->f_type   = 0;
  fsb->f_bsize  = fsd.du_bsize;
  fsb->f_blocks = fsd.du_fsize - fsd.du_isize;
  fsb->f_bfree  = fsd.du_tfree;
  fsb->f_bavail = fsd.du_tfree;
  fsb->f_files  = (fsd.du_isize - 2) * fsd.du_inopb;
  fsb->f_ffree  = fsd.du_tinode;
  fsb->f_fsid.val[0] = fsd.du_site;
  fsb->f_fsid.val[1] = fsd.du_pckno;
  return 0;
}
#endif /* _AIX && _I386 */
