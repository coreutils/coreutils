/* Declarations for GNU's read utmp module.
   Copyright (C) 92, 93, 94, 95, 96, 1997 Free Software Foundation, Inc.

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

/* Written by jla; revised by djm */

#ifndef __READUTMP_H__
# define __READUTMP_H__

# include <stdio.h>
# include <sys/types.h>

# ifdef HAVE_UTMPX_H
#  include <utmpx.h>
#  define STRUCT_UTMP struct utmpx
#  define UT_TIME_MEMBER(UT_PTR) ((UT_PTR)->ut_tv.tv_sec)
# else
#  include <utmp.h>
#  define STRUCT_UTMP struct utmp
#  define UT_TIME_MEMBER(UT_PTR) ((UT_PTR)->ut_time)
# endif

# include <time.h>
# ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>
# endif

# include <errno.h>
# ifndef errno
extern int errno;
# endif

# if !defined (UTMP_FILE) && defined (_PATH_UTMP)
#  define UTMP_FILE _PATH_UTMP
# endif

# if !defined (WTMP_FILE) && defined (_PATH_WTMP)
#  define WTMP_FILE _PATH_WTMP
# endif

# ifdef UTMPX_FILE /* Solaris, SysVr4 */
#  undef UTMP_FILE
#  define UTMP_FILE UTMPX_FILE
# endif

# ifdef WTMPX_FILE /* Solaris, SysVr4 */
#  undef WTMP_FILE
#  define WTMP_FILE WTMPX_FILE
# endif

# ifndef UTMP_FILE
#  define UTMP_FILE "/etc/utmp"
# endif

# ifndef WTMP_FILE
#  define WTMP_FILE "/etc/wtmp"
# endif

extern STRUCT_UTMP * utmp_contents;

# undef PARAMS
# if defined (__STDC__) && __STDC__
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif

extern char * extract_trimmed_name PARAMS((const STRUCT_UTMP *ut));
extern int read_utmp PARAMS((const char *filename));

#endif /* __READUTMP_H__ */
