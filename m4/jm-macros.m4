#serial 9

dnl Misc type-related macros for fileutils, sh-utils, textutils.

AC_DEFUN(jm_MACROS,
[
  AC_PREREQ(2.14a)

  GNU_PACKAGE="GNU $PACKAGE"
  AC_DEFINE_UNQUOTED(GNU_PACKAGE, "$GNU_PACKAGE",
    [The concatenation of the strings `GNU ', and PACKAGE.])
  AC_SUBST(GNU_PACKAGE)

  dnl This macro actually runs replacement code.  See isc-posix.m4.
  AC_REQUIRE([AC_ISC_POSIX])dnl

  jm_INCLUDED_REGEX([lib/regex.c])

  AC_REQUIRE([jm_BISON])
  AC_REQUIRE([jm_ASSERT])
  AC_REQUIRE([jm_AC_HEADER_INTTYPES_H])
  AC_REQUIRE([jm_STRUCT_UTIMBUF])
  AC_REQUIRE([jm_STRUCT_DIRENT_D_TYPE])
  AC_REQUIRE([jm_STRUCT_DIRENT_D_INO])
  AC_REQUIRE([jm_CHECK_DECLS])

  AC_REQUIRE([jm_PREREQ])

  AC_REQUIRE([jm_FUNC_LCHOWN])
  AC_REQUIRE([jm_FUNC_CHOWN])
  AC_REQUIRE([jm_FUNC_MKTIME])
  AC_REQUIRE([jm_FUNC_LSTAT])
  AC_REQUIRE([jm_FUNC_STAT])
  AC_REQUIRE([jm_FUNC_REALLOC])
  AC_REQUIRE([jm_FUNC_MALLOC])
  AC_REQUIRE([jm_FUNC_READDIR])
  AC_REQUIRE([jm_FUNC_MEMCMP])
  AC_REQUIRE([jm_FUNC_GLIBC_UNLOCKED_IO])
  AC_REQUIRE([jm_FUNC_FNMATCH])
  AC_REQUIRE([jm_AFS])
  AC_REQUIRE([jm_AC_PREREQ_XSTRTOUMAX])
  AC_REPLACE_FUNCS(strcasecmp strncasecmp)
  AC_REPLACE_FUNCS(dup2)

  dnl used by e.g. intl/*domain.c and lib/canon-host.c
  AC_REPLACE_FUNCS(strdup)

  AC_REPLACE_FUNCS(memchr)
  AC_REPLACE_FUNCS(memmove)
  AC_CHECK_FUNCS(getpagesize)

  # By default, argmatch should fail calling usage (1).
  AC_DEFINE(ARGMATCH_DIE, [usage (1)],
	    [Define to the function xargmatch calls on failures.])
  AC_DEFINE(ARGMATCH_DIE_DECL, [extern void usage ()],
	    [Define to the declaration of the xargmatch failure function.])

  dnl Used to define SETVBUF in sys2.h.
  dnl This evokes the following warning from autoconf:
  dnl ...: warning: AC_TRY_RUN called without default to allow cross compiling
  AC_FUNC_SETVBUF_REVERSED

  AM_FUNC_GETLINE
  if test $am_cv_func_working_getline != yes; then
    AC_CHECK_FUNCS(getdelim)
  fi

])

AC_DEFUN(jm_CHECK_ALL_TYPES,
[
  AC_TYPE_GETGROUPS
  AC_TYPE_MODE_T
  AC_TYPE_OFF_T
  AC_TYPE_PID_T
  AC_TYPE_SIGNAL
  AC_TYPE_SIZE_T
  AC_TYPE_UID_T
  AC_CHECK_TYPE(ino_t, unsigned long)
  AC_TYPE_SSIZE_T
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
  AC_REQUIRE([jm_AC_TYPE_UNSIGNED_LONG_LONG])
])
