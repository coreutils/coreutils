#serial 2

dnl From Paul Eggert.

# Define HAVE_ST_MTIM if struct stat has an st_mtim member.

AC_DEFUN(AC_STRUCT_ST_MTIM,
 [AC_CACHE_CHECK([for st_mtim in struct stat], ac_cv_struct_st_mtim,
   [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>], [struct stat s; s.st_mtim;],
     ac_cv_struct_st_mtim=yes,
     ac_cv_struct_st_mtim=no)])

  if test $ac_cv_struct_st_mtim = yes; then
    if test x = y; then
      # This code is deliberately never run via ./configure.
      # FIXME: this is a hack to make autoheader put the corresponding
      # HAVE_* undef for this symbol in config.h.in.  This saves me the
      # trouble of having to add the #undef in acconfig.h manually.
      AC_CHECK_FUNCS(ST_MTIM)
    fi
    # Defining it this way (rather than via AC_DEFINE) short-circuits the
    # autoheader check -- autoheader doesn't know it's already been taken
    # care of by the hack above.
    ac_kludge=HAVE_ST_MTIM
    AC_DEFINE_UNQUOTED($ac_kludge)
  fi
 ]
)
