/* Declarations for GNU's read utmp module.
   Copyright (C) 1992-2002 Free Software Foundation, Inc.

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

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

# include <sys/types.h>

/* AIX 4.3.3 has both utmp.h and utmpx.h, but only struct utmp
   has the ut_exit member.  */
# if HAVE_UTMPX_H && HAVE_UTMP_H && HAVE_STRUCT_UTMP_UT_EXIT && ! HAVE_STRUCT_UTMPX_UT_EXIT
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
#   define UT_USER(Utmp) Utmp->ut_user
#  endif
#  if HAVE_STRUCT_UTMP_UT_NAME
#   undef UT_USER
#   define UT_USER(Utmp) Utmp->ut_name
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

# define HAVE_STRUCT_XTMP_UT_TYPE \
    (HAVE_STRUCT_UTMP_UT_TYPE \
     || HAVE_STRUCT_UTMPX_UT_TYPE)

typedef struct UTMP_STRUCT_NAME STRUCT_UTMP;

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

# undef PARAMS
# if defined (__STDC__) && __STDC__
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif

extern char *extract_trimmed_name PARAMS ((const STRUCT_UTMP *ut));
extern int read_utmp PARAMS ((const char *filename,
			      int *n_entries, STRUCT_UTMP **utmp_buf));

#endif /* __READUTMP_H__ */
