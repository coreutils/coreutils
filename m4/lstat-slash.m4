#serial 1

dnl From Jim Meyering.
dnl FIXME

AC_DEFUN(jm_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK,
[
 AC_CACHE_CHECK(
  [whether lstat dereferences a symlink specified with a trailing slash],
  jm_cv_func_lstat_dereferences_slashed_symlink,
  [
   rm -f conftest.sym conftest.file
   : > conftest.file
   if ln -s conftest.file conftest.sym; then
     AC_TRY_RUN([
#     include <sys/types.h>
#     include <sys/stat.h>

      int
      main ()
      {
        struct stat sbuf;
        exit (lstat ("conftest.sym/", &sbuf) ? 0 : 1);
      }
      ],
      jm_cv_func_lstat_dereferences_slashed_symlink=yes,
      jm_cv_func_lstat_dereferences_slashed_symlink=no,
      dnl When crosscompiling, be pessimistic so we'll end up using the
      dnl replacement version of lstat that checkes for trailing slashes
      dnl and calls lstat a second time when necessary.
      jm_cv_func_lstat_dereferences_slashed_symlink=no
     )
  ])

  if test $jm_cv_func_lstat_dereferences_slashed_symlink = yes; then
    AC_SUBST(LIBOBJS)
    LIBOBJS="$LIBOBJS lstat.$ac_objext"
    AC_DEFINE_UNQUOTED(LSTAT_FOLLOWS_SLASHED_SYMLINK, 1,
      [Define if lstat dereferences a symlink specified with a trailing slash])
  fi
])
