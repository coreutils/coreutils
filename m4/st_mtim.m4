#serial 1

dnl From Paul Eggert.

# Define HAVE_ST_MTIM if struct stat has an st_mtim member.

AC_DEFUN(AC_STRUCT_ST_MTIM,
 [AC_CACHE_CHECK([for st_mtim in struct stat], ac_cv_struct_st_mtim,
   [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>], [struct stat s; s.st_mtim;],
     ac_cv_struct_st_mtim=yes, ac_cv_struct_st_mtim=no)])
  if test $ac_cv_struct_st_mtim = yes; then
   AC_DEFINE(HAVE_ST_MTIM)
  fi])
