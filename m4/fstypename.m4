#serial 3

dnl From Jim Meyering.
dnl
dnl See if struct statfs has the f_fstypename member.
dnl If so, define HAVE_F_FSTYPENAME_IN_STATFS.
dnl

AC_DEFUN([jm_FSTYPENAME],
  [
    AC_CACHE_CHECK([for f_fstypename in struct statfs],
		   fu_cv_sys_f_fstypename_in_statfs,
      [
	AC_TRY_COMPILE(
	  [
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
	  ],
	  [struct statfs s; int i = sizeof s.f_fstypename;],
	  fu_cv_sys_f_fstypename_in_statfs=yes,
	  fu_cv_sys_f_fstypename_in_statfs=no
	)
      ]
    )

    if test $fu_cv_sys_f_fstypename_in_statfs = yes; then
      AC_DEFINE(HAVE_F_FSTYPENAME_IN_STATFS, 1,
		[Define if struct statfs has the f_fstypename member.])
    fi
  ]
)
