#serial 1

dnl Find out how to get the file descriptor associated with an open DIR*.
dnl From Jim Meyering

AC_DEFUN([UTILS_FUNC_DIRFD],
[
  AC_REPLACE_FUNCS([dirfd])
  AC_CHECK_DECLS([dirfd])
  if test $ac_cv_func_dirfd = no; then
    AC_CACHE_CHECK(
	      [how to get the file descriptor associated with an open DIR*],
		   ac_cv_sys_dir_to_fd,
      [
        dirfd_save_DEFS=$DEFS
	for ac_expr in						\
								\
	    '# Solaris'						\
	    'dir_p->d_fd'					\
								\
	    '# Solaris'						\
	    'dir_p->dd_fd'					\
								\
	    '# systems for which the info is not available'	\
	    -1							\
	    ; do

	  # Skip each embedded comment.
	  case "$ac_expr" in '#'*) continue;; esac

	  DEFS="$DEFS -DDIR_TO_FD=$ac_expr"
	  AC_TRY_COMPILE(
	    [#include <sys/types.h>
#include <dirent.h>
	    ],
	    [DIR *dir_p = opendir("."); (void) ($ac_expr);],
	    dir_fd_done=yes
	  )
	  DEFS=$dirfd_save_DEFS
	  test "$dir_fd_done" = yes && break
	done

	ac_cv_sys_dir_to_fd=$ac_expr
      ]
    )
    AC_DEFINE_UNQUOTED(DIR_TO_FD,
      $ac_cv_sys_dir_to_fd,
      [the file descriptor associated with `dir_p'])
  fi
])
