#serial 5

dnl Misc lib-related macros for fileutils, sh-utils, textutils.

AC_DEFUN([jm_LIB_CHECK],
[

  # Check for libypsec.a on Dolphin M88K machines.
  AC_CHECK_LIB(ypsec, main)

  # m88k running dgux 5.4 needs this
  AC_CHECK_LIB(ldgc, main)

  # Some programs need to link with -lm.  printf does if it uses
  # lib/strtod.c which uses pow.  And seq uses the math functions,
  # floor, modf, rint.  And factor uses sqrt.  And sleep uses fesetround.

  # Save a copy of $LIBS and add $FLOOR_LIBM before these tests
  # Check for these math functions used by seq.
  ac_su_saved_lib="$LIBS"
  LIBS="$LIBS -lm"
  AC_CHECK_FUNCS(floor modf rint)
  LIBS="$ac_su_saved_lib"

  AC_SUBST(SQRT_LIBM)
  AC_CHECK_FUNCS(sqrt)
  if test $ac_cv_func_sqrt = no; then
    AC_CHECK_LIB(m, sqrt, [SQRT_LIBM=-lm])
  fi

  AC_SUBST(FESETROUND_LIBM)
  AC_CHECK_FUNCS(fesetround)
  if test $ac_cv_func_fesetround = no; then
    AC_CHECK_LIB(m, fesetround, [FESETROUND_LIBM=-lm])
  fi

  # The -lsun library is required for YP support on Irix-4.0.5 systems.
  # m88k/svr3 DolphinOS systems using YP need -lypsec for id.
  AC_SEARCH_LIBS(yp_match, [sun ypsec])

  # SysV needs -lsec, older versions of Linux need -lshadow for
  # shadow passwords.  UnixWare 7 needs -lgen.
  AC_SEARCH_LIBS(getspnam, [shadow sec gen])

  AC_CHECK_HEADERS(shadow.h)

  # Requirements for su.c.
  shadow_includes="\
$ac_includes_default
#if HAVE_SHADOW_H
# include <shadow.h>
#endif
"
  AC_CHECK_MEMBERS([struct spwd.sp_pwdp],,,[$shadow_includes])
  AC_CHECK_FUNCS(getspnam)

  # SCO-ODT-3.0 is reported to need -lufc for crypt.
  # NetBSD needs -lcrypt for crypt.
  ac_su_saved_lib="$LIBS"
  AC_SEARCH_LIBS(crypt, [ufc crypt], [LIB_CRYPT="$ac_cv_search_crypt"])
  LIBS="$ac_su_saved_lib"
  AC_SUBST(LIB_CRYPT)
])
