#serial 1

dnl From Jim Meyering.
dnl FIXME: describe

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
     # If the `ln -s' command failed, then we probably don't even
     # have an lstat function.
     jm_cv_func_lstat_dereferences_slashed_symlink=no
   fi
  ])

  test $jm_cv_func_lstat_dereferences_slashed_symlink = yes \
    && zero_one=1 \
    || zero_one=0
  AC_DEFINE_UNQUOTED(LSTAT_FOLLOWS_SLASHED_SYMLINK, $zero_one,
    [Define if lstat dereferences a symlink specified with a trailing slash])

  if test $jm_cv_func_lstat_dereferences_slashed_symlink = no; then
    AC_SUBST(LIBOBJS)
    # Append lstat.o if it's not already in $LIBOBJS.
    case "$LIBOBJS" in
      *lstat.$ac_objext*) ;;
      *) LIBOBJS="$LIBOBJS lstat.$ac_objext" ;;
    esac
  fi
])
