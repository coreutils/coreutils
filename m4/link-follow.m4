#serial 3
dnl Run a program to determine whether whether link(2) follows symlinks.
dnl Set LINK_FOLLOWS_SYMLINKS accordingly.

AC_DEFUN([jm_AC_FUNC_LINK_FOLLOWS_SYMLINK],
[dnl
  AC_CACHE_CHECK(
    [whether link(2) dereferences a symlink specified with a trailing slash],
		 jm_ac_cv_func_link_follows_symlink,
  [
    dnl poor-man's AC_REQUIRE: FIXME: repair this once autoconf-3 provides
    dnl the appropriate framework.
    test -z "$ac_cv_header_unistd_h" \
      && AC_CHECK_HEADERS(unistd.h)

    # Create a regular file.
    echo > conftest.file
    AC_TRY_RUN(
      [
#       include <sys/types.h>
#       include <sys/stat.h>
#       ifdef HAVE_UNISTD_H
#        include <unistd.h>
#       endif

#       define SAME_INODE(Stat_buf_1, Stat_buf_2) \
	  ((Stat_buf_1).st_ino == (Stat_buf_2).st_ino \
	   && (Stat_buf_1).st_dev == (Stat_buf_2).st_dev)

	int
	main ()
	{
	  const char *file = "conftest.file";
	  const char *sym = "conftest.sym";
	  const char *hard = "conftest.hard";
	  struct stat sb_file, sb_hard;

	  /* Create a symlink to the regular file. */
	  if (symlink (file, sym))
	    abort ();

	  /* Create a hard link to that symlink.  */
	  if (link (sym, hard))
	    abort ();

	  if (lstat (hard, &sb_hard))
	    abort ();
	  if (lstat (file, &sb_file))
	    abort ();

	  /* If the dev/inode of hard and file are the same, then
	     the link call followed the symlink.  */
	  return SAME_INODE (sb_hard, sb_file) ? 0 : 1;
	}
      ],
      jm_ac_cv_func_link_follows_symlink=yes,
      jm_ac_cv_func_link_follows_symlink=no,
      jm_ac_cv_func_link_follows_symlink=yes dnl We're cross compiling.
    )
  ])
  if test $jm_ac_cv_func_link_follows_symlink = yes; then
    AC_DEFINE(LINK_FOLLOWS_SYMLINKS, 1,
      [Define if `link(2)' dereferences symbolic links.])
  fi
])
