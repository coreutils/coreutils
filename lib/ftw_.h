/* Copyright (C) 1992, 1996, 1997, 1998, 1999, 2003
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/*
 *	X/Open Portability Guide 4.2: ftw.h
 */

#ifndef _FTW_H
# define _FTW_H	1

# ifdef _LIBC
#  include <features.h>
# else
#  undef __THROW
#  define __THROW
#  undef __BEGIN_DECLS
#  define __BEGIN_DECLS
#  undef __END_DECLS
#  define __END_DECLS
# endif

# include <sys/types.h>
# include <sys/stat.h>

/* When compiling stand-alone on a system that does not define
   __USE_XOPEN_EXTENDED, define that symbol so that all the
   required declarations appear.  */
# if ! defined _LIBC && ! defined __USE_XOPEN_EXTENDED
#  define FTW_H_STANDALONE 1
#  define __USE_XOPEN_EXTENDED 1
# endif

__BEGIN_DECLS

/* Values for the FLAG argument to the user function passed to `ftw'
   and 'nftw'.  */
enum
{
  FTW_F,		/* Regular file.  */
# define FTW_F	 FTW_F
  FTW_D,		/* Directory.  */
# define FTW_D	 FTW_D
  FTW_DNR,		/* Unreadable directory.  */
# define FTW_DNR	 FTW_DNR
  FTW_NS,		/* Unstatable file.  */
# define FTW_NS	 FTW_NS

  /* Can't chdir to named directory.  This can happen only when using
     FTW_CHDIR.  Note that although we can't chdir into that directory,
     we were able to stat it, so SB (2nd argument to callback) is valid.  */
  FTW_DCH,
# define FTW_DCH FTW_DCH

  /* Can't chdir to parent of named directory.  This can happen only when
     using FTW_CHDIR.  Unlike for FTW_DCH, in this case, SB is not valid.
     In fact, it is NULL.  */
  FTW_DCHP,
# define FTW_DCHP FTW_DCHP

  /* nftw calls the user-supplied function at most twice for each directory
     it encounters.  When calling it the first time, it passes this value
     as the `type'.  */
  FTW_DPRE,
# define FTW_DPRE FTW_DPRE

# if defined __USE_BSD || defined __USE_XOPEN_EXTENDED

  FTW_SL,		/* Symbolic link.  */
#  define FTW_SL	 FTW_SL
# endif

# ifdef __USE_XOPEN_EXTENDED
/* These flags are only passed from the `nftw' function.  */
  FTW_DP,		/* Directory, all subdirs have been visited. */
#  define FTW_DP	 FTW_DP
  FTW_SLN		/* Symbolic link naming non-existing file.  */
#  define FTW_SLN FTW_SLN

# endif	/* extended X/Open */
};


# ifdef __USE_XOPEN_EXTENDED
/* Flags for fourth argument of `nftw'.  */
enum
{
  FTW_PHYS = 1,		/* Perform physical walk, ignore symlinks.  */
#  define FTW_PHYS	FTW_PHYS
  FTW_MOUNT = 2,	/* Report only files on same file system as the
			   argument.  */
#  define FTW_MOUNT	FTW_MOUNT
  FTW_CHDIR = 4,	/* Change to current directory while processing it.  */
#  define FTW_CHDIR	FTW_CHDIR
  FTW_DEPTH = 8		/* Report files in directory before directory itself.*/
#  define FTW_DEPTH	FTW_DEPTH
};

/* Structure used for fourth argument to callback function for `nftw'.  */
struct FTW
  {
    int base;
    int level;
    int skip;
  };
# endif	/* extended X/Open */


/* Convenient types for callback functions.  */
typedef int (*__ftw_func_t) (const char *__filename,
			     const struct stat *__status, int __flag);
# ifdef __USE_LARGEFILE64
typedef int (*__ftw64_func_t) (const char *__filename,
			       const struct stat64 *__status, int __flag);
# endif
# ifdef __USE_XOPEN_EXTENDED
typedef int (*__nftw_func_t) (const char *__filename,
			      const struct stat *__status, int __flag,
			      struct FTW *__info);
#  ifdef __USE_LARGEFILE64
typedef int (*__nftw64_func_t) (const char *__filename,
				const struct stat64 *__status,
				int __flag, struct FTW *__info);
#  endif
# endif

/* Call a function on every element in a directory tree.  */
# ifndef __USE_FILE_OFFSET64
extern int ftw (const char *__dir, __ftw_func_t __func, int __descriptors)
     __THROW;
# else
#  ifdef __REDIRECT
extern int __REDIRECT (ftw, (const char *__dir, __ftw_func_t __func,
			     int __descriptors) __THROW, ftw64);
#  else
#   define ftw ftw64
#  endif
# endif
# ifdef __USE_LARGEFILE64
extern int ftw64 (const char *__dir, __ftw64_func_t __func,
		  int __descriptors) __THROW;
# endif

# ifdef __USE_XOPEN_EXTENDED
/* Call a function on every element in a directory tree.  FLAG allows
   to specify the behaviour more detailed.  */
#  ifndef __USE_FILE_OFFSET64
extern int nftw (const char *__dir, __nftw_func_t __func, int __descriptors,
		 int __flag) __THROW;
#  else
#   ifdef __REDIRECT
extern int __REDIRECT (nftw, (const char *__dir, __nftw_func_t __func,
			      int __descriptors, int __flag) __THROW, nftw64);
#   else
#    define nftw nftw64
#   endif
#  endif
#  ifdef __USE_LARGEFILE64
extern int nftw64 (const char *__dir, __nftw64_func_t __func,
		   int __descriptors, int __flag) __THROW;
#  endif
# endif

/* If we defined __USE_XOPEN_EXTENDED above, undefine it here.  */
# ifdef FTW_H_STANDALONE
#  undef __USE_XOPEN_EXTENDED
# endif

__END_DECLS

#endif	/* ftw.h */
