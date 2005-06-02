/* Declarations for GNU's read utmp module.

   Copyright (C) 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
   2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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

/* Written by jla; revised by djm */

#ifndef __READUTMP_H__
# define __READUTMP_H__

# include <sys/types.h>

/* AIX 4.3.3 has both utmp.h and utmpx.h, but only struct utmp
   has the ut_exit member.  */
# if (HAVE_UTMPX_H && HAVE_UTMP_H && HAVE_STRUCT_UTMP_UT_EXIT \
      && ! HAVE_STRUCT_UTMPX_UT_EXIT)
#  undef HAVE_UTMPX_H
# endif

# ifdef HAVE_UTMPX_H
#  ifdef HAVE_UTMP_H
    /* HPUX 10.20 needs utmp.h, for the definition of e.g., UTMP_FILE.  */
#   include <utmp.h>
#  endif
#  include <utmpx.h>
#  define UTMP_STRUCT_NAME utmpx
#  define UT_TIME_MEMBER(UT_PTR) ((UT_PTR)->ut_tv.tv_sec)
#  define SET_UTMP_ENT setutxent
#  define GET_UTMP_ENT getutxent
#  define END_UTMP_ENT endutxent
#  ifdef HAVE_UTMPXNAME
#   define UTMP_NAME_FUNCTION utmpxname
#  endif

#  if HAVE_STRUCT_UTMPX_UT_EXIT_E_TERMINATION
#   define UT_EXIT_E_TERMINATION(U) ((U)->ut_exit.e_termination)
#  else
#   if HAVE_STRUCT_UTMPX_UT_EXIT_UT_TERMINATION
#    define UT_EXIT_E_TERMINATION(U) ((U)->ut_exit.ut_termination)
#   else
#    define UT_EXIT_E_TERMINATION(U) 0
#   endif
#  endif

#  if HAVE_STRUCT_UTMPX_UT_EXIT_E_EXIT
#   define UT_EXIT_E_EXIT(U) ((U)->ut_exit.e_exit)
#  else
#   if HAVE_STRUCT_UTMPX_UT_EXIT_UT_EXIT
#    define UT_EXIT_E_EXIT(U) ((U)->ut_exit.ut_exit)
#   else
#    define UT_EXIT_E_EXIT(U) 0
#   endif
#  endif

# else
#  include <utmp.h>
#  if !HAVE_DECL_GETUTENT
    struct utmp *getutent();
#  endif
#  define UTMP_STRUCT_NAME utmp
#  define UT_TIME_MEMBER(UT_PTR) ((UT_PTR)->ut_time)
#  define SET_UTMP_ENT setutent
#  define GET_UTMP_ENT getutent
#  define END_UTMP_ENT endutent
#  ifdef HAVE_UTMPNAME
#   define UTMP_NAME_FUNCTION utmpname
#  endif

#  if HAVE_STRUCT_UTMP_UT_EXIT_E_TERMINATION
#   define UT_EXIT_E_TERMINATION(U) ((U)->ut_exit.e_termination)
#  else
#   if HAVE_STRUCT_UTMP_UT_EXIT_UT_TERMINATION
#    define UT_EXIT_E_TERMINATION(U) ((U)->ut_exit.ut_termination)
#   else
#    define UT_EXIT_E_TERMINATION(U) 0
#   endif
#  endif

#  if HAVE_STRUCT_UTMP_UT_EXIT_E_EXIT
#   define UT_EXIT_E_EXIT(U) ((U)->ut_exit.e_exit)
#  else
#   if HAVE_STRUCT_UTMP_UT_EXIT_UT_EXIT
#    define UT_EXIT_E_EXIT(U) ((U)->ut_exit.ut_exit)
#   else
#    define UT_EXIT_E_EXIT(U) 0
#   endif
#  endif

# endif

/* Accessor macro for the member named ut_user or ut_name.  */
# ifdef HAVE_UTMPX_H

#  if HAVE_STRUCT_UTMPX_UT_USER
#   define UT_USER(Utmp) ((Utmp)->ut_user)
#  endif
#  if HAVE_STRUCT_UTMPX_UT_NAME
#   undef UT_USER
#   define UT_USER(Utmp) ((Utmp)->ut_name)
#  endif

# else

#  if HAVE_STRUCT_UTMP_UT_USER
#   define UT_USER(Utmp) ((Utmp)->ut_user)
#  endif
#  if HAVE_STRUCT_UTMP_UT_NAME
#   undef UT_USER
#   define UT_USER(Utmp) ((Utmp)->ut_name)
#  endif

# endif

# define HAVE_STRUCT_XTMP_UT_EXIT \
    (HAVE_STRUCT_UTMP_UT_EXIT \
     || HAVE_STRUCT_UTMPX_UT_EXIT)

# define HAVE_STRUCT_XTMP_UT_ID \
    (HAVE_STRUCT_UTMP_UT_ID \
     || HAVE_STRUCT_UTMPX_UT_ID)

# define HAVE_STRUCT_XTMP_UT_PID \
    (HAVE_STRUCT_UTMP_UT_PID \
     || HAVE_STRUCT_UTMPX_UT_PID)

typedef struct UTMP_STRUCT_NAME STRUCT_UTMP;

enum { UT_USER_SIZE = sizeof UT_USER ((STRUCT_UTMP *) 0) };

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

# if HAVE_STRUCT_XTMP_UT_PID
#  define UT_PID(U) ((U)->ut_pid)
# else
#  define UT_PID(U) 0
# endif

# if HAVE_STRUCT_UTMP_UT_TYPE || HAVE_STRUCT_UTMPX_UT_TYPE
#  define UT_TYPE_EQ(U, V) ((U)->ut_type == (V))
#  define UT_TYPE_NOT_DEFINED 0
# else
#  define UT_TYPE_EQ(U, V) 0
#  define UT_TYPE_NOT_DEFINED 1
# endif

# ifdef BOOT_TIME
#  define UT_TYPE_BOOT_TIME(U) UT_TYPE_EQ (U, BOOT_TIME)
# else
#  define UT_TYPE_BOOT_TIME(U) 0
# endif

# ifdef USER_PROCESS
#  define UT_TYPE_USER_PROCESS(U) UT_TYPE_EQ (U, USER_PROCESS)
# else
#  define UT_TYPE_USER_PROCESS(U) 0
# endif

# define IS_USER_PROCESS(U)					\
   (UT_USER (U)[0]						\
    && (UT_TYPE_USER_PROCESS (U)				\
        || (UT_TYPE_NOT_DEFINED && UT_TIME_MEMBER (U) != 0)))

/* Options for read_utmp.  */
enum
  {
    READ_UTMP_CHECK_PIDS = 1
  };

char *extract_trimmed_name (const STRUCT_UTMP *ut);
int read_utmp (char const *file, size_t *n_entries, STRUCT_UTMP **utmp_buf,
	       int options);

#endif /* __READUTMP_H__ */
