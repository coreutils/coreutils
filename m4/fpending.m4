#serial 3

dnl From Jim Meyering
dnl Using code from emacs, based on suggestions from Paul Eggert
dnl and Ulrich Drepper.

dnl Find out how to determine the number of pending output bytes on a stream.
dnl glibc (2.1.93 and newer) and Solaris provide __fpending.  On other systems,
dnl we have to grub around in the FILE struct.

AC_DEFUN([jm_FUNC_FPENDING],
[
  AC_CHECK_HEADERS(stdio_ext.h)
  AC_REPLACE_FUNCS([__fpending])
  fp_headers='
#     if HAVE_STDIO_EXT_H
#      include <stdio_ext.h>
#     endif
'
  AC_CHECK_DECLS([__fpending], , , $fp_headers)
  if test $ac_cv_func___fpending = no; then
    AC_CACHE_CHECK(
	      [how to determine the number of pending output bytes on a stream],
		   ac_cv_sys_pending_output_n_bytes,
      [
	for ac_expr in						\
								\
	    '# glibc2'						\
	    'fp->_IO_write_ptr - fp->_IO_write_base'		\
								\
	    '# traditional Unix'				\
	    'fp->_ptr - fp->_base'				\
								\
	    '# BSD'						\
	    'fp->_p - fp->_bf._base'				\
								\
	    '# SCO, Unixware'					\
	    'fp->__ptr - fp->__base'				\
								\
	    '# old glibc?'					\
	    'fp->__bufp - fp->__buffer'				\
								\
	    '# old glibc iostream?'				\
	    'fp->_pptr - fp->_pbase'				\
								\
	    '# VMS'						\
	    '(*fp)->_ptr - (*fp)->_base'			\
								\
	    '# e.g., DGUX R4.11; the info is not available'	\
	    1							\
	    ; do

	  # Skip each embedded comment.
	  case "$ac_expr" in '#'*) continue;; esac

	  AC_TRY_COMPILE(
	    [#include <stdio.h>
	    ],
	    [FILE *fp = stdin; (void) ($ac_expr);],
	    fp_done=yes
	  )
	  test "$fp_done" = yes && break
	done

	ac_cv_sys_pending_output_n_bytes=$ac_expr
      ]
    )
    AC_DEFINE_UNQUOTED(PENDING_OUTPUT_N_BYTES,
      $ac_cv_sys_pending_output_n_bytes,
      [the number of pending output bytes on stream `fp'])
  fi
])
