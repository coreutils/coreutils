#serial 49   -*- autoconf -*-

dnl Misc type-related macros for fileutils, sh-utils, textutils.

AC_DEFUN([jm_MACROS],
[
  AC_PREREQ(2.53b)

  GNU_PACKAGE="GNU $PACKAGE"
  AC_DEFINE_UNQUOTED(GNU_PACKAGE, "$GNU_PACKAGE",
    [The concatenation of the strings `GNU ', and PACKAGE.])
  AC_SUBST(GNU_PACKAGE)

  AM_MISSING_PROG(HELP2MAN, help2man)
  AC_SUBST(OPTIONAL_BIN_PROGS)
  AC_SUBST(OPTIONAL_BIN_ZCRIPTS)
  AC_SUBST(MAN)
  AC_SUBST(DF_PROG)

  dnl This macro actually runs replacement code.  See isc-posix.m4.
  AC_REQUIRE([AC_ISC_POSIX])dnl

  jm_CHECK_ALL_TYPES
  jm_INCLUDED_REGEX([lib/regex.c])

  AC_REQUIRE([UTILS_HOST_OS])
  AC_REQUIRE([UTILS_FUNC_MKDIR_TRAILING_SLASH])
  AC_REQUIRE([jm_BISON])
  AC_REQUIRE([jm_ASSERT])
  AC_REQUIRE([jm_CHECK_TYPE_STRUCT_UTIMBUF])
  AC_REQUIRE([jm_CHECK_TYPE_STRUCT_DIRENT_D_TYPE])
  AC_REQUIRE([jm_CHECK_TYPE_STRUCT_DIRENT_D_INO])
  AC_REQUIRE([jm_CHECK_DECLS])

  AC_REQUIRE([jm_PREREQ])

  AC_REQUIRE([UTILS_FUNC_DIRFD])
  AC_REQUIRE([AC_FUNC_ACL])
  AC_REQUIRE([jm_FUNC_LCHOWN])
  AC_REQUIRE([fetish_FUNC_RMDIR_NOTEMPTY])
  AC_REQUIRE([jm_FUNC_CHOWN])
  AC_REQUIRE([jm_FUNC_MKTIME])
  AC_REQUIRE([jm_FUNC_LSTAT])
  AC_REQUIRE([AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK])
  AC_REQUIRE([jm_FUNC_STAT])
  AC_REQUIRE([AC_FUNC_REALLOC])
  AC_REQUIRE([AC_FUNC_MALLOC])
  AC_REQUIRE([AC_FUNC_STRERROR_R])
  AC_REQUIRE([jm_FUNC_NANOSLEEP])
  AC_REQUIRE([jm_FUNC_MEMCMP])
  AC_REQUIRE([jm_FUNC_GLIBC_UNLOCKED_IO])
  AC_REQUIRE([AC_FUNC_FNMATCH_GNU])
  AC_REQUIRE([jm_FUNC_GROUP_MEMBER])
  AC_REQUIRE([jm_FUNC_PUTENV])
  AC_REQUIRE([jm_AFS])
  AC_REQUIRE([jm_AC_PREREQ_XSTRTOUMAX])
  AC_REQUIRE([jm_AC_PREREQ_XSTRTOIMAX])
  AC_REQUIRE([jm_AC_FUNC_LINK_FOLLOWS_SYMLINK])
  AC_REQUIRE([AC_FUNC_ERROR_AT_LINE])
  AC_REQUIRE([jm_FUNC_GNU_STRFTIME])
  AC_REQUIRE([jm_FUNC_MKTIME])
  AC_REQUIRE([jm_FUNC_FPENDING])

  AC_REQUIRE([jm_FUNC_GETGROUPS])
  test -n "$GETGROUPS_LIB" && LIBS="$GETGROUPS_LIB $LIBS"

  AC_REQUIRE([AC_FUNC_FSEEKO])
  AC_REQUIRE([AC_FUNC_VPRINTF])
  AC_REQUIRE([AC_FUNC_ALLOCA])
  AC_FUNC_GETLOADAVG([lib])
  AC_REQUIRE([jm_SYS_PROC_UPTIME])
  AC_REQUIRE([jm_FUNC_FTRUNCATE])
  AC_REQUIRE([vb_FUNC_RENAME])

  AC_REPLACE_FUNCS(strcasecmp strncasecmp)
  AC_REPLACE_FUNCS(dup2)
  AC_REPLACE_FUNCS(gethostname getusershell)
  AC_REPLACE_FUNCS(sig2str)
  AC_REPLACE_FUNCS(strcspn stpcpy strstr strtol strtoul)
  AC_REPLACE_FUNCS(strpbrk)
  AC_REPLACE_FUNCS(euidaccess memcmp rmdir rpmatch strndup strverscmp)
  AC_REPLACE_FUNCS(atexit)
  AC_REPLACE_FUNCS(getpass)

  dnl used by e.g. intl/*domain.c and lib/canon-host.c
  AC_REPLACE_FUNCS(strdup)

  AC_REPLACE_FUNCS(memchr memcpy memmove memrchr memset)
  AC_CHECK_FUNCS(getpagesize)

  AC_REQUIRE([UTILS_FUNC_MKSTEMP])

  # By default, argmatch should fail calling usage (1).
  AC_DEFINE(ARGMATCH_DIE, [usage (1)],
	    [Define to the function xargmatch calls on failures.])
  AC_DEFINE(ARGMATCH_DIE_DECL, [extern void usage ()],
	    [Define to the declaration of the xargmatch failure function.])

  dnl Used to define SETVBUF in sys2.h.
  dnl This evokes the following warning from autoconf:
  dnl ...: warning: AC_TRY_RUN called without default to allow cross compiling
  AC_FUNC_SETVBUF_REVERSED

  # used by sleep and shred
  # Solaris 2.5.1 needs -lposix4 to get the clock_gettime function.
  # Solaris 7 prefers the library name -lrt to the obsolescent name -lposix4.

  # Save and restore LIBS so e.g., -lrt, isn't added to it.  Otherwise, *all*
  # programs in the package would end up linked with that potentially-shared
  # library, inducing unnecessary run-time overhead.
  fetish_saved_libs=$LIBS
    AC_SEARCH_LIBS(clock_gettime, [rt posix4],
		   [LIB_CLOCK_GETTIME=$ac_cv_search_clock_gettime])
    AC_SUBST(LIB_CLOCK_GETTIME)
    AC_CHECK_FUNCS(clock_gettime clock_settime)
  LIBS=$fetish_saved_libs
  AC_CHECK_FUNCS(gettimeofday)
  AC_FUNC_GETTIMEOFDAY_CLOBBER

  AC_REQUIRE([AC_FUNC_CLOSEDIR_VOID])
  AC_REQUIRE([jm_FUNC_UTIME])

  AC_CHECK_FUNCS( \
    bcopy \
    canonicalize_file_name \
    endgrent \
    endpwent \
    fchdir \
    fdatasync \
    ftime \
    ftruncate \
    getcwd \
    gethrtime \
    getmntinfo \
    hasmntopt \
    isascii \
    iswspace \
    lchown \
    listmntent \
    localeconv \
    memcpy \
    mempcpy \
    mkfifo \
    realpath \
    resolvepath \
    sethostname \
    strchr \
    strerror \
    strrchr \
    sysctl \
    sysinfo \
    wcrtomb \
    tzset \
  )

  AM_FUNC_GETLINE
  if test $am_cv_func_working_getline != yes; then
    AC_CHECK_FUNCS(getdelim)
  fi
  AC_FUNC_OBSTACK

  AC_FUNC_STRTOD

  # See if linking `seq' requires -lm.
  # It does on nearly every system.  The single exception (so far) is
  # BeOS which has all the math functions in the normal runtime library
  # and doesn't have a separate math library.

  AC_SUBST(SEQ_LIBM)
  ac_seq_body='
     static double x, y;
     x = floor (x);
     x = rint (x);
     x = modf (x, &y);'
  AC_TRY_LINK([#include <math.h>], $ac_seq_body, ,
    [ac_seq_save_LIBS="$LIBS"
     LIBS="$LIBS -lm"
     AC_TRY_LINK([#include <math.h>], $ac_seq_body, SEQ_LIBM=-lm)
     LIBS="$ac_seq_save_LIBS"
    ])

  AM_LANGINFO_CODESET
  jm_GLIBC21
  AM_ICONV
  jm_FUNC_UNLINK_BUSY_TEXT

  # These tests are for df.
  jm_LIST_MOUNTED_FILESYSTEMS([list_mounted_fs=yes], [list_mounted_fs=no])
  jm_FSTYPENAME
  jm_FILE_SYSTEM_USAGE([space=yes], [space=no])
  if test $list_mounted_fs = yes && test $space = yes; then
    DF_PROG=df
    AC_LIBOBJ(fsusage)
    AC_LIBOBJ(mountlist)
  fi
  AC_REQUIRE([jm_AC_DOS])

])

# These tests must be run before any use of AC_CHECK_TYPE,
# because that macro compiles code that tests e.g., HAVE_UNISTD_H.
# See the definition of ac_includes_default in `configure'.
AC_DEFUN([jm_CHECK_ALL_HEADERS],
[
  AC_CHECK_HEADERS( \
    errno.h  \
    fcntl.h \
    fenv.h \
    float.h \
    hurd.h \
    limits.h \
    memory.h \
    mntent.h \
    mnttab.h \
    netdb.h \
    paths.h \
    stdlib.h \
    stddef.h \
    stdint.h \
    string.h \
    sys/filsys.h \
    sys/fs/s5param.h \
    sys/fs_types.h \
    sys/fstyp.h \
    sys/ioctl.h \
    sys/mntent.h \
    sys/mount.h \
    sys/param.h \
    sys/resource.h \
    sys/socket.h \
    sys/statfs.h \
    sys/statvfs.h \
    sys/sysctl.h \
    sys/systeminfo.h \
    sys/time.h \
    sys/timeb.h \
    sys/vfs.h \
    sys/wait.h \
    syslog.h \
    termios.h \
    unistd.h \
    utime.h \
    values.h \
  )
])

# This macro must be invoked before any tests that run the compiler.
AC_DEFUN([jm_CHECK_ALL_TYPES],
[
  dnl This test must come as early as possible after the compiler configuration
  dnl tests, because the choice of the file model can (in principle) affect
  dnl whether functions and headers are available, whether they work, etc.
  AC_REQUIRE([AC_SYS_LARGEFILE])

  dnl This test must precede tests of compiler characteristics like
  dnl that for the inline keyword, since it may change the degree to
  dnl which the compiler supports such features.
  AC_REQUIRE([AM_C_PROTOTYPES])

  dnl Checks for typedefs, structures, and compiler characteristics.
  AC_REQUIRE([AC_C_BIGENDIAN])
  AC_REQUIRE([AC_C_CONST])
  AC_REQUIRE([AC_C_VOLATILE])
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([AC_C_LONG_DOUBLE])

  AC_REQUIRE([jm_CHECK_ALL_HEADERS])
  AC_REQUIRE([AC_HEADER_DIRENT])
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_MEMBERS(
    [struct stat.st_author,
     struct stat.st_blksize],,,
    [$ac_includes_default
#include <sys/stat.h>
  ])
  AC_REQUIRE([AC_STRUCT_ST_BLOCKS])

  AC_REQUIRE([AC_STRUCT_TM])
  AC_REQUIRE([AC_STRUCT_TIMEZONE])
  AC_REQUIRE([AC_HEADER_STAT])
  AC_REQUIRE([AC_STRUCT_ST_MTIM_NSEC])
  AC_REQUIRE([AC_STRUCT_ST_DM_MODE])
  AC_REQUIRE([jm_CHECK_TYPE_STRUCT_TIMESPEC])

  AC_REQUIRE([AC_TYPE_GETGROUPS])
  AC_REQUIRE([AC_TYPE_MODE_T])
  AC_REQUIRE([AC_TYPE_OFF_T])
  AC_REQUIRE([AC_TYPE_PID_T])
  AC_REQUIRE([AC_TYPE_SIGNAL])
  AC_REQUIRE([AC_TYPE_SIZE_T])
  AC_REQUIRE([AC_TYPE_UID_T])
  AC_CHECK_TYPE(ino_t, unsigned long)

  dnl This relies on the fact that autoconf 2.14a's implementation of
  dnl AC_CHECK_TYPE checks includes unistd.h.
  AC_CHECK_TYPE(ssize_t, int)
  AC_CHECK_TYPE(major_t, unsigned int)
  AC_CHECK_TYPE(minor_t, unsigned int)

  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
  AC_REQUIRE([jm_AC_TYPE_UNSIGNED_LONG_LONG])

  AC_REQUIRE([AC_HEADER_MAJOR])
  AC_REQUIRE([AC_HEADER_DIRENT])

])
