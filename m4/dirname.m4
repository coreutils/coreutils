#serial 1

dnl See if the dirname function modifies its argument.
dnl We can't just do AC_REPLACE_FUNCS(dirname) because some systems
dnl (e.g., X/Open) have a function by that name that modifies and returns
dnl its argument.
AC_DEFUN(jm_FUNC_DIRNAME,
[dnl
  AC_CACHE_CHECK([for working dirname function], jm_cv_func_working_dirname,
  [AC_TRY_RUN([
    int main ()
    {
      const char *path = "a/b";
      char *dir = dirname (path);
      exit ((dir != path && *dir == 'a' && dir[1] == 0) ? 0 : 1);
    }
    ], jm_cv_func_working_dirname=yes dnl The library version works.
    , jm_cv_func_working_dirname=no dnl The library version does NOT work.
    , jm_cv_func_working_dirname=no dnl We're cross compiling.
    )
  ])

  if test $jm_cv_func_working_dirname = no; then
    LIBOBJS="$LIBOBJS dirname.o"
    AC_SUBST(LIBOBJS)dnl
  fi
])
