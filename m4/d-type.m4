#serial 5

dnl From Jim Meyering.
dnl
dnl Check whether struct dirent has a member named d_type.
dnl

AC_DEFUN([jm_CHECK_TYPE_STRUCT_DIRENT_D_TYPE],
  [AC_REQUIRE([AC_HEADER_DIRENT])dnl
   AC_CACHE_CHECK([for d_type member in directory struct],
		  jm_cv_struct_dirent_d_type,
     [AC_TRY_LINK(dnl
       [
#include <sys/types.h>
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#else /* not HAVE_DIRENT_H */
# define dirent direct
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H */
       ],
       [struct dirent dp; dp.d_type = 0;],

       jm_cv_struct_dirent_d_type=yes,
       jm_cv_struct_dirent_d_type=no)
     ]
   )
   if test $jm_cv_struct_dirent_d_type = yes; then
     AC_DEFINE(HAVE_STRUCT_DIRENT_D_TYPE, 1,
  [Define if there is a member named d_type in the struct describing
   directory headers.])
   fi
  ]
)
