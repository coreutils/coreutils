#serial 5

dnl Find out how to get the file descriptor associated with an open DIR*.
dnl From Jim Meyering

AC_DEFUN([UTILS_FUNC_DIRFD],
[
  dnl Work around a bug of AC_EGREP_CPP in autoconf-2.57.
  AC_REQUIRE([AC_PROG_CPP])
  AC_REQUIRE([AC_PROG_EGREP])

  AC_HEADER_DIRENT
  dirfd_headers='
#if HAVE_DIRENT_H
# include <dirent.h>
#else /* not HAVE_DIRENT_H */
# define dirent direct
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# if HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H */
'
  AC_CHECK_FUNCS(dirfd)
  AC_CHECK_DECLS([dirfd], , , $dirfd_headers)

  AC_CACHE_CHECK([whether dirfd is a macro],
    jm_cv_func_dirfd_macro,
    AC_EGREP_CPP([dirent_header_defines_dirfd], [$dirfd_headers
#ifdef dirfd
 dirent_header_defines_dirfd
#endif],
      jm_cv_func_dirfd_macro=yes,
      jm_cv_func_dirfd_macro=no))

  # Use the replacement only if we have no function, macro,
  # or declaration with that name.
  if test $ac_cv_func_dirfd,$ac_cv_have_decl_dirfd,$jm_cv_func_dirfd_macro \
      = no,no,no; then
    AC_REPLACE_FUNCS([dirfd])
    AC_CACHE_CHECK(
	      [how to get the file descriptor associated with an open DIR*],
		   gl_cv_sys_dir_fd_member_name,
      [
        dirfd_save_CFLAGS=$CFLAGS
	for ac_expr in d_fd dd_fd; do

	  CFLAGS="$CFLAGS -DDIR_FD_MEMBER_NAME=$ac_expr"
	  AC_TRY_COMPILE(
	    [$dirfd_headers
	    ],
	    [DIR *dir_p = opendir("."); (void) dir_p->DIR_FD_MEMBER_NAME;],
	    dir_fd_found=yes
	  )
	  CFLAGS=$dirfd_save_CFLAGS
	  test "$dir_fd_found" = yes && break
	done
	test "$dir_fd_found" = yes || ac_expr=no_such_member

	gl_cv_sys_dir_fd_member_name=$ac_expr
      ]
    )
    if test $gl_cv_sys_dir_fd_member_name != no_such_member; then
      AC_DEFINE_UNQUOTED(DIR_FD_MEMBER_NAME,
	$gl_cv_sys_dir_fd_member_name,
	[the name of the file descriptor member of DIR])
    fi
    AH_VERBATIM(DIR_TO_FD,
		[#ifdef DIR_FD_MEMBER_NAME
# define DIR_TO_FD(Dir_p) ((Dir_p)->DIR_FD_MEMBER_NAME)
#else
# define DIR_TO_FD(Dir_p) -1
#endif
]
    )
  fi
])
