#serial 1

dnl From Jim Meyering.
dnl
dnl See if struct statfs has the f_fstypename member.
dnl If so, define HAVE_F_FSTYPENAME_IN_STATFS.
dnl

AC_DEFUN(jm_FSTYPENAME,
  [
    AC_CACHE_CHECK([for f_fstypename in struct statfs],
		   fu_cv_sys_f_fstypename_in_statfs,
      [
	AC_TRY_COMPILE(
	  [
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
      if test x = y; then
	# This code is deliberately never run via ./configure.
	# FIXME: this is a hack to make autoheader put the corresponding
	# HAVE_* undef for this symbol in config.h.in.  This saves me the
	# trouble of having to maintain the #undef in acconfig.h manually.
	AC_CHECK_FUNCS(F_FSTYPENAME_IN_STATFS)
      fi
      # Defining it this way (rather than via AC_DEFINE) short-circuits the
      # autoheader check -- autoheader doesn't know it's already been taken
      # care of by the hack above.
      ac_kludge=HAVE_F_FSTYPENAME_IN_STATFS
      AC_DEFINE_UNQUOTED($ac_kludge)
    fi
  ]
)
