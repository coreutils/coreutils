#serial 7

dnl From Jim Meyering.
dnl Determine whether chown accepts arguments of -1 for uid and gid.
dnl If it doesn't, arrange to use the replacement function.
dnl

AC_DEFUN([jm_FUNC_CHOWN],
[AC_REQUIRE([AC_TYPE_UID_T])dnl
 test -z "$ac_cv_header_unistd_h" \
   && AC_CHECK_HEADERS(unistd.h)
 AC_CACHE_CHECK([for working chown], jm_cv_func_working_chown,
  [AC_TRY_RUN([
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#   ifdef HAVE_UNISTD_H
#    include <unistd.h>
#   endif

    int
    main ()
    {
      char *f = "conftest.chown";
      struct stat before, after;

      if (creat (f, 0600) < 0)
        exit (1);
      if (stat (f, &before) < 0)
        exit (1);
      if (chown (f, (uid_t) -1, (gid_t) -1) == -1)
        exit (1);
      if (stat (f, &after) < 0)
        exit (1);
      exit ((before.st_uid == after.st_uid
	     && before.st_gid == after.st_gid) ? 0 : 1);
    }
	      ],
	     jm_cv_func_working_chown=yes,
	     jm_cv_func_working_chown=no,
	     dnl When crosscompiling, assume chown is broken.
	     jm_cv_func_working_chown=no)
  ])
  if test $jm_cv_func_working_chown = no; then
    AC_LIBOBJ(chown)
    AC_DEFINE(chown, rpl_chown,
      [Define to rpl_chown if the replacement function should be used.])
  fi
])
