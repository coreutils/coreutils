#serial 1
# Arrange to define HAVE_MMAP_STACK and to compile mmap-stack.c
# if there is sufficient support.
# From Jim Meyering

AC_DEFUN([AC_SYS_MMAP_STACK],
[
  # prerequisites
  AC_REQUIRE([AC_FUNC_MMAP])
  AC_CHECK_HEADERS_ONCE(sys/mman.h ucontext.h stdarg.h)
  AC_CHECK_FUNCS_ONCE(getcontext makecontext setcontext)

  # For now, require tmpfile. FIXME: if there's a system with working mmap
  # and *context functions yet that lacks tmpfile, we can provide a replacement.
  AC_CHECK_FUNCS_ONCE(tmpfile)

  ac_i=$ac_cv_func_tmpfile
  ac_i=$ac_i:$ac_cv_func_getcontext
  ac_i=$ac_i:$ac_cv_func_makecontext
  ac_i=$ac_i:$ac_cv_func_setcontext
  ac_i=$ac_i:$ac_cv_func_mmap_fixed_mapped

  if test $ac_i = yes:yes:yes:yes:yes; then
    AC_LIBOBJ(mmap-stack)
    AC_DEFINE(HAVE_MMAP_STACK, 1,
      [Define to 1 if there is sufficient support (mmap, getcontext,
       makecontext, setcontext) for running a process with mmap'd
       memory for its stack.])
  fi
])
