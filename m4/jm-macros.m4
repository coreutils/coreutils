#serial 7

dnl Misc type-related macros for fileutils, sh-utils, textutils.

AC_DEFUN(jm_MACROS,
[
  AC_PREREQ(2.13)               dnl Minimum Autoconf version required.

  GNU_PACKAGE="GNU $PACKAGE"
  AC_DEFINE_UNQUOTED(GNU_PACKAGE, "$GNU_PACKAGE",
    [The concatenation of the strings \`GNU ', and PACKAGE.])
  AC_SUBST(GNU_PACKAGE)

  dnl This macro actually runs replacement code.  See isc-posix.m4.
  AC_REQUIRE([AC_ISC_POSIX])dnl

  jm_INCLUDED_REGEX([lib/regex.c])

  AC_REQUIRE([jm_ASSERT])
  AC_REQUIRE([jm_AC_HEADER_INTTYPES_H])
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
  AC_CHECK_TYPE(ssize_t, int)
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
  AC_REPLACE_FUNCS(strcasecmp strncasecmp)

  # By default, argmatch should fail calling usage (1).
  AC_DEFINE(ARGMATCH_DIE, [usage (1)],
	    [Define to the function xargmatch calls on failures.])
  AC_DEFINE(ARGMATCH_DIE_DECL, [extern void usage ()],
	    [Define to the declaration of the xargmatch failure function.])

  dnl Used to define SETVBUF in sys2.h.
  AC_FUNC_SETVBUF_REVERSED

])
