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
        /* Linux will dereference the symlink and fail.
           That is better in the sense that it means we will not
           have to compile and use the lstat wrapper.  */
        exit (lstat ("conftest.sym/", &sbuf) ? 0 : 1);
      }
      ],
      jm_cv_func_lstat_dereferences_slashed_symlink=yes,
      jm_cv_func_lstat_dereferences_slashed_symlink=no,
      dnl When crosscompiling, be pessimistic so we will end up using the
      dnl replacement version of lstat that checkes for trailing slashes
      dnl and calls lstat a second time when necessary.
      jm_cv_func_lstat_dereferences_slashed_symlink=no
     )
   else
     jm_cv_func_lstat_dereferences_slashed_symlink=no
   fi
  ])

  # FIXME: convert to 0 or 1.
  AC_DEFINE_UNQUOTED(LSTAT_FOLLOWS_SLASHED_SYMLINK, FIXME,
    [Define if lstat dereferences a symlink specified with a trailing slash])

  if test $jm_cv_func_lstat_dereferences_slashed_symlink = no; then
    AC_SUBST(LIBOBJS)
# FIXME: append to LIBOBJS only if it's not there already.
    LIBOBJS="$LIBOBJS lstat.$ac_objext"
    AC_DEFINE_UNQUOTED(LSTAT_FOLLOWS_SLASHED_SYMLINK, 1,
      [Define if lstat dereferences a symlink specified with a trailing slash])
  fi
])
