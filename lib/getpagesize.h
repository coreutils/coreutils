/* Emulate getpagesize on systems that lack it.
   Copyright (C) 1999, 2000, 2004 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.  */

#ifndef HAVE_GETPAGESIZE

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if !defined getpagesize && defined _SC_PAGESIZE
# if !(defined VMS && __VMS_VER < 70000000)
#  define getpagesize() sysconf (_SC_PAGESIZE)
# endif
#endif

#if !defined getpagesize && defined VMS
# ifdef __ALPHA
#  define getpagesize() 8192
# else
#  define getpagesize() 512
# endif
#endif

/* This is for BeOS.  */
#if !defined getpagesize && HAVE_OS_H
# include <OS.h>
# if defined B_PAGE_SIZE
#  define getpagesize() B_PAGE_SIZE
# endif
#endif

#if !defined getpagesize && HAVE_SYS_PARAM_H
# include <sys/param.h>
# ifdef EXEC_PAGESIZE
#  define getpagesize() EXEC_PAGESIZE
# else
#  ifdef NBPG
#   ifndef CLSIZE
#    define CLSIZE 1
#   endif
#   define getpagesize() (NBPG * CLSIZE)
#  else
#   ifdef NBPC
#    define getpagesize() NBPC
#   endif
#  endif
# endif
#endif

#endif /* not HAVE_GETPAGESIZE */
