#serial 1

dnl SunOS's readdir is broken in such a way that rm.c has to add extra code
dnl to test whether a NULL return value really means there are no more files
dnl in the directory.
dnl
dnl Detect the problem by creating a directory containing 300 files (254 not
dnl counting . and .. is the minimum) and see if a loop doing `readdir; unlink'
dnl removes all of them.
dnl
dnl Define HAVE_WORKING_READDIR if readdir does *not* have this problem.

dnl Written by Jim Meyering.

AC_DEFUN(jm_FUNC_READDIR,
[dnl
AC_REQUIRE([AC_HEADER_DIRENT])
AC_CHECK_HEADERS(string.h)
AC_CACHE_CHECK([for working readdir], jm_cv_func_working_readdir,
  [dnl
  # Arrange for deletion of the temporary directory this test creates, in
  # case the test itself fails to delete everything -- as happens on Sunos.
  ac_clean_files="$ac_clean_files conf-dir"

  AC_TRY_RUN(
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
	abort ();
      if (chdir (dir))
	abort ();

      for (i = 0; i < 300; i++)
	{
	  char file_name[4];
	  FILE *out;

	  sprintf (file_name, "%03d", i);
	  out = fopen (file_name, "w");
	  if (!out)
	    abort ();
	  if (fclose (out) == EOF)
	    abort ();
	}

      if (chdir (".."))
	abort ();
    }

    static void
    remove_dir (const char *dir)
    {
      DIR *dirp;

      if (chdir (dir))
	abort ();

      dirp = opendir (".");
      if (dirp == NULL)
	abort ();

      while (1)
	{
	  struct dirent *dp = readdir (dirp);
	  if (dp == NULL)
	    break;

	  if (DOT_OR_DOTDOT (dp->d_name))
	    continue;

	  if (unlink (dp->d_name))
	    abort ();
	}
      closedir (dirp);

      if (chdir (".."))
	abort ();

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
