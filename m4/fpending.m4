#serial 1

dnl From Jim Meyering
dnl Using code from emacs, based on suggestions from Paul Eggert
dnl and Ulrich Drepper.

AC_DEFUN(jm_FUNC_FPENDING,
[
  AC_CHECK_HEADERS(stdio_ext.h)
  AC_REPLACE_FUNCS([__fpending])
  if test $ac_cv_func___fpending = no; then
    AC_CACHE_CHECK(
	      [how to determine the number of pending output bytes on a stream],
		   ac_cv_sys_pending_output_n_bytes,
      [
        fp_save_DEFS=$DEFS
	for ac_expr in 						\
								\
	  '# glibc2'						\
	  'fp->_IO_write_ptr - fp->_IO_write_base' 		\
								\
	  '# traditional Unix'					\
	  'fp->_ptr - fp->_base' 				\
								\
	  '# BSD'						\
	  'fp->_p - fp->_bf._base' 				\
								\
	  '# SCO, Unixware'					\
	  'fp->__ptr - fp->__base' 				\
								\
	  '# old glibc?'					\
	  'fp->__bufp - fp->__buffer' 				\
								\
	  '# old glibc iostream?'				\
	  'fp->_pptr - fp->_pbase' 				\
								\
	  '# VMS'						\
	  '(*fp)->_ptr - (*fp)->_base' 				\
								\
	  '# e.g., DGUX R4.11; the info is not available'	\
	  1 							\
	  ; do
	    # Skip each embedded comment.
	    case "$ac_expr" in '#'*) continue;; esac
'

	    DEFS="$DEFS -DPENDING_OUTPUT_N_BYTES=$ac_expr"
	    AC_TRY_COMPILE(
	      [#include <stdio.h>
	      ],
	      [long unsigned int n = $ac_expr;],
	      fp_done=yes
            )
	    DEFS=$fp_save_DEFS
	    test "$fp_done" = yes && break
	done
	AC_DEFINE_UNQUOTED(PENDING_OUTPUT_N_BYTES, $ac_expr,
	  [the number of pending output bytes on stream `fp'])
      ])
  fi




  if test $fu_cv_sys_struct_timespec = yes; then
    AC_DEFINE_UNQUOTED(HAVE_STRUCT_TIMESPEC, 1,
		       [Define if struct timespec is declared in <time.h>. ])
  fi
])
