/* system-dependent definitions for fileutils programs.
   Copyright (C) 1989, 1990, 1991 Free Software Foundation, Inc.

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

/* Include sys/types.h before this file.  */

#include <sys/stat.h>

#ifdef	STAT_MACROS_BROKEN
#ifdef S_ISBLK
#undef S_ISBLK
#endif
#ifdef S_ISCHR
#undef S_ISCHR
#endif
#ifdef S_ISDIR
#undef S_ISDIR
#endif
#ifdef S_ISFIFO
#undef S_ISFIFO
#endif
#ifdef S_ISLNK
#undef S_ISLNK
#endif
#ifdef S_ISMPB
#undef S_ISMPB
#endif
#ifdef S_ISMPC
#undef S_ISMPC
#endif
#ifdef S_ISNWK
#undef S_ISNWK
#endif
#ifdef S_ISREG
#undef S_ISREG
#endif
#ifdef S_ISSOCK
#undef S_ISSOCK
#endif
#endif	/* STAT_MACROS_BROKEN.  */

#ifndef S_ISREG			/* Doesn't have POSIX.1 stat stuff. */
#define mode_t unsigned short
#endif
#if !defined(S_ISBLK) && defined(S_IFBLK)
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#endif
#if !defined(S_ISCHR) && defined(S_IFCHR)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#endif
#if !defined(S_ISDIR) && defined(S_IFDIR)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#if !defined(S_ISREG) && defined(S_IFREG)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISFIFO) && defined(S_IFIFO)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#endif
#if !defined(S_ISLNK) && defined(S_IFLNK)
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif
#if !defined(S_ISSOCK) && defined(S_IFSOCK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#endif
#if !defined(S_ISMPB) && defined(S_IFMPB) /* V7 */
#define S_ISMPB(m) (((m) & S_IFMT) == S_IFMPB)
#define S_ISMPC(m) (((m) & S_IFMT) == S_IFMPC)
#endif
#if !defined(S_ISNWK) && defined(S_IFNWK) /* HP/UX */
#define S_ISNWK(m) (((m) & S_IFMT) == S_IFNWK)
#endif
#if !defined(HAVE_MKFIFO)
#define mkfifo(path, mode) (mknod ((path), (mode) | S_IFIFO, 0))
#endif

#ifndef _POSIX_SOURCE
#include <sys/param.h>
#endif

/* <unistd.h> should be included before any preprocessor test
   of _POSIX_VERSION.  */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "pathmax.h"

#ifndef _POSIX_VERSION
off_t lseek ();
#endif

#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif

/* Since major is a function on SVR4, we can't use `ifndef major'.  */
#ifdef MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#define HAVE_MAJOR
#endif
#ifdef MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#define HAVE_MAJOR
#endif
#ifdef major			/* Might be defined in sys/types.h.  */
#define HAVE_MAJOR
#endif

#ifndef HAVE_MAJOR
#define major(dev)  (((dev) >> 8) & 0xff)
#define minor(dev)  ((dev) & 0xff)
#define makedev(maj, min)  (((maj) << 8) | (min))
#endif
#undef HAVE_MAJOR

#ifdef _POSIX_VERSION
#include <utime.h>
#else /* not _POSIX_VERSION */
struct utimbuf
{
  long actime;
  long modtime;
};
#endif /* _POSIX_VERSION */

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#ifndef index
#define index strchr
#endif
#ifndef rindex
#define rindex strrchr
#endif
#ifndef bcopy
#define bcopy(from, to, len) memcpy ((to), (from), (len))
#endif
#ifndef bzero
#define bzero(s, n) memset ((s), 0, (n))
#endif
#else
#include <strings.h>
#endif

#include <errno.h>
#ifdef STDC_HEADERS
#define getopt system_getopt
#include <stdlib.h>
#undef getopt
#else /* not STDC_HEADERS */
char *getenv ();
extern int errno;
#endif /* STDC_HEADERS */

#if defined(HAVE_FCNTL_H) || defined(_POSIX_VERSION)
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif
#ifndef F_OK
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
#endif

#if defined(DIRENT) || defined(_POSIX_VERSION)
#include <dirent.h>
#define NLENGTH(direct) (strlen((direct)->d_name))
#else /* not (DIRENT or _POSIX_VERSION) */
#define dirent direct
#define NLENGTH(direct) ((direct)->d_namlen)
#ifdef SYSNDIR
#include <sys/ndir.h>
#endif /* SYSNDIR */
#ifdef SYSDIR
#include <sys/dir.h>
#endif /* SYSDIR */
#ifdef NDIR
#include <ndir.h>
#endif /* NDIR */
#endif /* DIRENT or _POSIX_VERSION */

#ifdef VOID_CLOSEDIR
/* Fake a return value. */
#define CLOSEDIR(d) (closedir (d), 0)
#else
#define CLOSEDIR(d) closedir (d)
#endif

/* Get or fake the disk device blocksize.
   Usually defined by sys/param.h (if at all).  */
#ifndef DEV_BSIZE
#ifdef BSIZE
#define DEV_BSIZE BSIZE
#else /* !BSIZE */
#define DEV_BSIZE 4096
#endif /* !BSIZE */
#endif /* !DEV_BSIZE */

/* Extract or fake data from a `struct stat'.
   ST_BLKSIZE: Optimal I/O blocksize for the file, in bytes.
   ST_NBLOCKS: Number of 512-byte blocks in the file
   (including indirect blocks). */
#ifndef HAVE_ST_BLOCKS
# define ST_BLKSIZE(statbuf) DEV_BSIZE
# if defined(_POSIX_SOURCE) || !defined(BSIZE) /* fileblocks.c uses BSIZE.  */
#  define ST_NBLOCKS(statbuf) (((statbuf).st_size + 512 - 1) / 512)
# else /* !_POSIX_SOURCE && BSIZE */
#  define ST_NBLOCKS(statbuf) (st_blocks ((statbuf).st_size))
# endif /* !_POSIX_SOURCE && BSIZE */
#else /* HAVE_ST_BLOCKS */
/* Some systems, like Sequents, return st_blksize of 0 on pipes. */
# define ST_BLKSIZE(statbuf) ((statbuf).st_blksize > 0 \
			       ? (statbuf).st_blksize : DEV_BSIZE)
# if defined(hpux) || defined(__hpux__) || defined(__hpux)
/* HP-UX counts st_blocks in 1024-byte units.
   This loses when mixing HP-UX and BSD filesystems with NFS.  */
#  define ST_NBLOCKS(statbuf) ((statbuf).st_blocks * 2)
# else /* !hpux */
#  if defined(_AIX) && defined(_I386)
/* AIX PS/2 counts st_blocks in 4K units.  */
#    define ST_NBLOCKS(statbuf) ((statbuf).st_blocks * 8)
#  else /* not AIX PS/2 */
#    define ST_NBLOCKS(statbuf) ((statbuf).st_blocks)
#  endif /* not AIX PS/2 */
# endif /* !hpux */
#endif /* HAVE_ST_BLOCKS */

/* Convert B 512-byte blocks to kilobytes if K is nonzero,
   otherwise return it unchanged. */
#define convert_blocks(b, k) ((k) ? ((b) + 1) / 2 : (b))

#ifndef S_ISLNK
#define lstat stat
#endif

#ifndef RETSIGTYPE
#define RETSIGTYPE void
#endif

#ifdef __GNUC__
#undef alloca
#define alloca __builtin_alloca
#else
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else
#ifndef _AIX
/* AIX alloca decl has to be the first thing in the file, bletch! */
char *alloca ();
#endif
#endif
#endif
