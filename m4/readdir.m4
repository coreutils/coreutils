#serial 1

dnl FIXME: describe

AC_DEFUN(jm_FUNC_READDIR,
[dnl
AC_REQUIRE([AC_HEADER_DIRENT])
AC_CHECK_HEADERS(string.h)
AC_CACHE_CHECK([for working readdir], jm_cv_func_working_readdir,
  [AC_TRY_RUN(
  changequote(<<, >>)dnl
  <<
#   include <stdio.h>
#   include <sys/types.h>
#   if HAVE_STRING_H
#    include <string.h>
#   endif

#   ifdef HAVE_DIRENT_H
#    include <dirent.h>
#    define NLENGTH(direct) (strlen((direct)->d_name))
#   else /* not HAVE_DIRENT_H */
#    define dirent direct
#    define NLENGTH(direct) ((direct)->d_namlen)
#    ifdef HAVE_SYS_NDIR_H
#     include <sys/ndir.h>
#    endif /* HAVE_SYS_NDIR_H */
#    ifdef HAVE_SYS_DIR_H
#     include <sys/dir.h>
#    endif /* HAVE_SYS_DIR_H */
#    ifdef HAVE_NDIR_H
#     include <ndir.h>
#    endif /* HAVE_NDIR_H */
#   endif /* HAVE_DIRENT_H */

#   define DOT_OR_DOTDOT(Basename) \
     (Basename[0] == '.' && (Basename[1] == '\0' \
			     || (Basename[1] == '.' && Basename[2] == '\0')))

    static void
    create_300_file_dir (const char *dir)
    {
      int i;

      if (mkdir (dir, 0700))
	exit (1);
      if (chdir (dir))
	exit (1);

      for (i = 0; i < 300; i++)
	{
	  char file_name[4];
	  FILE *out;

	  sprintf (file_name, "%03d", i);
	  out = fopen (file_name, "w");
	  if (!out)
	    exit (1);
	  if (fclose (out) == EOF)
	    exit (1);
	}

      if (chdir (".."))
	exit (1);
    }

    static void
    remove_dir (const char *dir)
    {
      DIR *dirp;

      if (chdir (dir))
	exit (1);

      dirp = opendir (".");
      if (dirp == NULL)
	exit (1);

      while (1)
	{
	  struct dirent *dp = readdir (dirp);
	  if (dp == NULL)
	    break;

	  if (DOT_OR_DOTDOT (dp->d_name))
	    continue;

	  if (unlink (dp->d_name))
	    exit (1);
	}
      closedir (dirp);

      if (chdir (".."))
	exit (1);

      if (rmdir (dir))
	exit (1);
    }

    int
    main ()
    {
      const char *dir = "conf-dir";
      create_300_file_dir (dir);
      remove_dir (dir);
      exit (0);
    }
  >>,
  changequote([, ])dnl
  jm_cv_func_working_readdir=yes,
  jm_cv_func_working_readdir=no,
  jm_cv_func_working_readdir=no)])

  if test x = y; then
    dnl This code is deliberately never run via ./configure.
    dnl FIXME: this is a gross hack to make autoheader put an entry
    dnl for this HAVE_-prefixed symbol in config.h.in.
    AC_CHECK_FUNCS(WORKING_READDIR)
  fi

  if test $jm_cv_func_working_readdir = yes; then
    ac_kludge=HAVE_WORKING_READDIR
    AC_DEFINE_UNQUOTED($ac_kludge)
  fi
])
