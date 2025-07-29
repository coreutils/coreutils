# -*- autoconf -*-
# Process this file with autoconf to produce a configure script.

# Copyright (C) 1991-2025 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

dnl Written by Jim Meyering.

AC_PREREQ([2.69])

# Make inter-release version strings look like, e.g., v6.9-219-g58ddd, which
# indicates that it is built from the 219th delta (in _some_ repository)
# following the v6.9 tag, and that 58ddd is a prefix of the commit SHA1.
# The explicit URL can be removed when autoconf > 2.69 is released.
AC_INIT([GNU coreutils],
        m4_esyscmd([build-aux/git-version-gen .tarball-version]),
        [bug-coreutils@gnu.org],
        [coreutils],
        [https://www.gnu.org/software/coreutils/])

AC_CONFIG_SRCDIR([src/ls.c])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([lib/config.h:lib/config.hin])

AM_INIT_AUTOMAKE([1.11.2 dist-xz color-tests parallel-tests subdir-objects])
AM_SILENT_RULES([yes]) # make --enable-silent-rules the default.

dnl POSIXCHECK is worthwhile for maintainers, but adds several seconds
dnl (more than 10% execution time) to ./configure, with no benefit for
dnl most users.  Using it to look for bugs requires:
dnl   GNULIB_POSIXCHECK=1 autoreconf -f
dnl   ./configure
dnl   make
dnl   make -C src clean
dnl   make CFLAGS=-DGNULIB_POSIXCHECK=1
dnl FIXME: Once we figure out how to avoid false positives, we should
dnl have 'make my-distcheck' in dist-check.mk exercise this.
m4_syscmd([test "${GNULIB_POSIXCHECK+set}" = set])
m4_if(m4_sysval, [0], [], [dnl
gl_ASSERT_NO_GNULIB_POSIXCHECK])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_RANLIB
AC_PROG_EGREP
AC_PROG_LN_S
gl_EARLY
gl_SET_CRYPTO_CHECK_DEFAULT([auto-gpl-compat])
gl_INIT
coreutils_MACROS

# These are safe, since 'sort', coreutils's only multithreaded app,
# does not use the relevant modules.
AC_DEFINE([GNULIB_EXCLUDE_SINGLE_THREAD], [1],
  [Define to 1 if apps call 'exclude' functions from a single thread.])
AC_DEFINE([GNULIB_REGEX_SINGLE_THREAD], [1],
  [Define to 1 if apps call 'regex' functions from a single thread.])
# Although 'sort' is multithreaded and can use these functions,
# it uses them only from the main thread.
AC_DEFINE([GNULIB_MBRTOWC_SINGLE_THREAD], [1],
  [Define to 1 if apps call mbrtowc, mbrtoc32, and 'regex' functions
   from a single thread.])
# This is safe, since all apps set locale early.
AC_DEFINE([GNULIB_WCHAR_SINGLE_LOCALE], [1],
  [Define to 1 if apps don't set the locale after calling
   locale-sensitive functions like mbrtowc and wcwidth.])

AC_DEFINE([GNULIB_MBRTOC32_REGULAR], [1],
  [Do not worry about rare encodings like CP864, EBCDIC, Johab, and Shift JIS
   that glibc does not support.])

# The test suite needs to know if we have a working perl.
AM_CONDITIONAL([HAVE_PERL], [test "$gl_cv_prog_perl" != no])

# gl_GCC_VERSION_IFELSE([major], [minor], [run-if-found], [run-if-not-found])
# ------------------------------------------------
# If $CPP is gcc-MAJOR.MINOR or newer, then run RUN-IF-FOUND.
# Otherwise, run RUN-IF-NOT-FOUND.
AC_DEFUN([gl_GCC_VERSION_IFELSE],
  [AC_PREPROC_IFELSE(
    [AC_LANG_PROGRAM(
      [[
#if ($1) < __GNUC__ || (($1) == __GNUC__ && ($2) <= __GNUC_MINOR__)
/* ok */
#else
# error "your version of gcc is older than $1.$2"
#endif
      ]]),
    ], [$3], [$4])
  ]
)

AC_ARG_ENABLE([single-binary],
  [AS_HELP_STRING([--enable-single-binary=[shebangs|symlinks]],
     [Compile all the tools in a single binary, reducing the overall size.
      When compiled this way, shebangs (default when enabled) or symlinks are
      installed for each tool that points to the single binary.])],
  [gl_single_binary=no ;
   case $enableval in
     yes) gl_single_binary=shebangs ;;
     no|shebangs|symlinks) gl_single_binary=$enableval ;;
     *)      AC_MSG_ERROR([bad value $enableval for single-binary option.
                           Options are: symlinks, shebangs, no.]) ;;
   esac],
  [gl_single_binary=no]
)
AC_ARG_ENABLE([single-binary-exceptions],
  [AS_HELP_STRING([--enable-single-binary-exceptions=PROG_LIST],
     [When used with --enable-single-binary, exclude the PROG_LIST from
      it, so these programs are compiled as separated files
      (comma-separated, default none))])],
  [gl_single_binary_exceptions=$enableval],
  [gl_single_binary_exceptions=]
)
if test "$gl_single_binary" = 'symlinks'; then
  if ! test "`echo ls | sed \"$program_transform_name\"`" = 'ls'; then
    AC_MSG_ERROR([program name transformations are not currently supported
                  with --enable-single-binary=symlinks.])
  fi
fi
AM_CONDITIONAL([SINGLE_BINARY], [test "$gl_single_binary" != no])

AC_ARG_ENABLE([bold-man-page-references],
  [AS_HELP_STRING([--disable-bold-man-page-references],
     [When generating man pages, do not apply bold style around any
      references like name(1) etc.])],
  [gl_bold_manpages=yes ;
   case $enableval in
     no|yes) gl_bold_manpages=$enableval ;;
     *)      AC_MSG_ERROR([bad value $enableval for bold-man-page-references.
                           Options are: yes, no.]) ;;
   esac],
  [gl_bold_manpages=yes]
)
AM_CONDITIONAL([BOLD_MAN_REFS], [test "$gl_bold_manpages" != no])

AC_ARG_ENABLE([gcc-warnings],
  [AS_HELP_STRING([--enable-gcc-warnings@<:@=TYPE@:>@],
    [control generation of GCC warnings.  The TYPE 'no' disables
     warnings (default for non-developer builds); 'yes' generates
     cheap warnings if available (default for developer builds);
     'expensive' in addition generates expensive-to-compute warnings
     if available.])],
  [case $enableval in
     no|yes|expensive) ;;
     *)      AC_MSG_ERROR([bad value $enableval for gcc-warnings option]) ;;
   esac
   gl_gcc_warnings=$enableval],
  [
   # GCC provides fine-grained control over diagnostics which
   # is used in gnulib for example to suppress warnings from
   # certain sections of code.  So if this is available and
   # we're running from a git repo, then auto enable the warnings.
   gl_gcc_warnings=no
   gl_GCC_VERSION_IFELSE([4], [6],
                         [test -d "$srcdir"/.git \
                          && ! test -f "$srcdir"/.tarball-version \
                          && gl_gcc_warnings=yes])]
)

# clang is unduly picky about some things, even by default.
if test "$gl_cv_compiler_clang" = yes; then
  gl_WARN_ADD([-Wno-format-extra-args])
  gl_WARN_ADD([-Wno-implicit-const-int-float-conversion])
  gl_WARN_ADD([-Wno-tautological-constant-out-of-range-compare])
fi

if test $gl_gcc_warnings != no; then
  gl_WARN_ADD([-Werror], [WERROR_CFLAGS])
  AC_SUBST([WERROR_CFLAGS])

  ew=
  AS_IF([test $gl_gcc_warnings != expensive],
    [# -fanalyzer and related options slow GCC considerably.
     ew="$ew -fanalyzer -Wno-analyzer-malloc-leak"])

  # This, $nw, is the list of warnings we disable.
  nw=$ew
  nw="$nw -Wstack-protector"        # not worth working around for pre GCC 15
  nw="$nw -Wformat-overflow=2"      # False alarms due to GCC bug 110333
  nw="$nw -Wformat-truncation=2"    # False alarm in ls.c, probably related
  nw="$nw -Winline"                 # system.h's readdir_ignoring_dot_and_dotdot

  # Suppress noreturn warnings with single binaries; otherwise
  # GCC complains about the renamed 'main' not being declared noreturn
  # because 'main_exit' calls 'exit' when linting.
  if test "$gl_single_binary" != no; then
    nw="$nw -Wsuggest-attribute=noreturn"
  fi

  # Using -Wstrict-overflow is a pain, but the alternative is worse.
  # For an example, see the code that provoked this report:
  # https://gcc.gnu.org/PR33498
  # Code like that still infloops with gcc-4.6.0 and -O2.  Scary indeed.

  gl_MANYWARN_ALL_GCC([ws])
  AS_VAR_APPEND([ws], [' -Wswitch-enum'])
  AS_VAR_APPEND([ws], [' -Wtrailing-whitespace'])
  gl_MANYWARN_COMPLEMENT([ws], [$ws], [$nw])
  for w in $ws; do
    gl_WARN_ADD([$w])
  done
  gl_WARN_ADD([-Wno-sign-compare])     # Too many warnings for now
  gl_WARN_ADD([-Wno-format-nonliteral])

  AC_SUBST([WARN_CFLAGS])

  AC_DEFINE([lint], [1], [Define to 1 if the compiler is checking for lint.])
  AH_VERBATIM([FORTIFY_SOURCE],
  [/* Enable compile-time and run-time bounds-checking, and some warnings,
      without upsetting glibc 2.15+. */
   #if !defined _FORTIFY_SOURCE && defined __OPTIMIZE__ && __OPTIMIZE__
   # define _FORTIFY_SOURCE 2
   #endif
])
  AC_DEFINE([GNULIB_PORTCHECK], [1], [enable some gnulib portability checks])

  # We use a slightly smaller set of warning options for lib/.
  # Remove the following and save the result in GNULIB_WARN_CFLAGS.
  nw=$ew
  nw="$nw -Wduplicated-branches"    # Too many false alarms
  nw="$nw -Wformat-truncation=2"
  nw="$nw -Wunused-macros"

  gl_MANYWARN_COMPLEMENT([GNULIB_WARN_CFLAGS], [$WARN_CFLAGS], [$nw])
  AC_SUBST([GNULIB_WARN_CFLAGS])

  # For gnulib-tests, the set is slightly smaller still.
  nw=
  # It's not worth being this picky about test programs.
  nw="$nw -Wmissing-variable-declarations"
  nw="$nw -Wsuggest-attribute=cold"
  nw="$nw -Wsuggest-attribute=const"
  nw="$nw -Wsuggest-attribute=format"
  nw="$nw -Wsuggest-attribute=pure"
  gl_MANYWARN_COMPLEMENT([GNULIB_TEST_WARN_CFLAGS],
                         [$GNULIB_WARN_CFLAGS], [$nw])
  AC_SUBST([GNULIB_TEST_WARN_CFLAGS])
fi

AC_FUNC_FORK

optional_bin_progs=
AC_CHECK_FUNCS([chroot],
        gl_ADD_PROG([optional_bin_progs], [chroot]))
AC_CHECK_FUNCS([gethostid],
        gl_ADD_PROG([optional_bin_progs], [hostid]))
AC_CHECK_FUNCS([sigsuspend],
        gl_ADD_PROG([optional_bin_progs], [timeout]))

gl_WINSIZE_IN_PTEM

AC_MSG_CHECKING([whether localtime caches TZ])
AC_CACHE_VAL([utils_cv_localtime_cache],
[if test x$ac_cv_func_tzset = xyes; then
AC_RUN_IFELSE([AC_LANG_SOURCE([[#include <time.h>
#if STDC_HEADERS
# include <stdlib.h>
#endif
extern char **environ;
void unset_TZ (void)
{
  char **from, **to;
  for (to = from = environ; (*to = *from); from++)
    if (! (to[0][0] == 'T' && to[0][1] == 'Z' && to[0][2] == '='))
      to++;
}
int
main ()
{
  time_t now = time ((time_t *) 0);
  int hour_GMT0, hour_unset;
  if (putenv ("TZ=GMT0") != 0)
    return 1;
  hour_GMT0 = localtime (&now)->tm_hour;
  unset_TZ ();
  hour_unset = localtime (&now)->tm_hour;
  if (putenv ("TZ=PST8") != 0)
    return 1;
  if (localtime (&now)->tm_hour == hour_GMT0)
    return 1;
  unset_TZ ();
  if (localtime (&now)->tm_hour != hour_unset)
    return 1;
  return 0;
}]])],
[utils_cv_localtime_cache=no],
[utils_cv_localtime_cache=yes],
[# If we have tzset, assume the worst when cross-compiling.
utils_cv_localtime_cache=yes])
else
        # If we lack tzset, report that localtime does not cache TZ,
        # since we can't invalidate the cache if we don't have tzset.
        utils_cv_localtime_cache=no
fi])dnl
AC_MSG_RESULT([$utils_cv_localtime_cache])
if test $utils_cv_localtime_cache = yes; then
  AC_DEFINE([LOCALTIME_CACHE], [1], [FIXME])
fi

# Find the library for dynamic loading of shared libraries.
AC_SEARCH_LIBS([dlopen], [dl])
AS_CASE([$ac_cv_search_dlopen],
  [no | 'none required'],
    [LIB_DL=],
  [*],
    [LIB_DL="$ac_cv_search_dlopen"])
AC_SUBST([LIB_DL])

# Should 'sort' link libcrypto dynamically?
AS_CASE([$LIB_CRYPTO],
  [-lcrypto],
    [# Check for dlopen and libcrypto dynamic linking in one program,
     # as there's little point to checking them separately.
     AC_CACHE_CHECK([for dlopen and whether libcrypto is linked dynamically],
       [utils_cv_libcrypto_soname],
       [utils_cv_libcrypto_soname=no
        saved_LIBS=$LIBS
        LIBS="$LIBS $LIB_DL $LIB_CRYPTO"
        AC_LINK_IFELSE(
          [AC_LANG_PROGRAM(
             [[#include <dlfcn.h>
               #include <openssl/sha.h>
               /* Use SHA512 rather than MD5 here to avoid deprecation warnings.
                  So need to check HAVE_OPENSSL_MD5.. with DLOPEN_LIBCRYPTO. */
             ]],
             [[return !(dlopen ("libcrypto.so", RTLD_LAZY | RTLD_GLOBAL)
                        && SHA512 (0, 0, 0));]])],
          [# readelf works with cross-builds; ldd works on more platforms.
           LIBCRYPTO_SONAME="`(readelf -d conftest$EXEEXT || ldd conftest$EXEEXT
                              ) 2>/dev/null |
                              sed -n 's/.*\(libcrypto\.so\.[[.0-9]]*\).*/\1/p'`"
           AS_CASE([$LIBCRYPTO_SONAME],
             [*libcrypto*],
               [utils_cv_libcrypto_soname=$LIBCRYPTO_SONAME])])
        LIBS=$saved_LIBS])
     AS_CASE([$utils_cv_libcrypto_soname],
       [*libcrypto*],
         [AC_DEFINE([DLOPEN_LIBCRYPTO], [1],
                    [Define to 1 if dlopen exists and libcrypto is
                     linked dynamically.])
          AC_DEFINE_UNQUOTED([LIBCRYPTO_SONAME], ["$utils_cv_libcrypto_soname"],
                             [versioned libcrypto])
         ])])

# macOS >= 10.12
AC_CHECK_FUNCS([fclonefileat])

# Assume that if getattrat exists, it's compatible with Solaris 11.
AC_CHECK_FUNCS([getattrat])
if test $ac_cv_func_getattrat = yes; then
  LIB_NVPAIR=-lnvpair
  AC_SUBST([LIB_NVPAIR])
fi

# glibc >= 2.28 and linux kernel >= 4.11, Android API level 30.
gl_CHECK_FUNCS_ANDROID([statx], [[#include <sys/stat.h>]])

# SCO-ODT-3.0 is reported to need -los to link programs using initgroups
AC_CHECK_FUNCS([initgroups])
if test $ac_cv_func_initgroups = no; then
  AC_CHECK_LIB([os], [initgroups])
fi

AC_CHECK_FUNCS([syslog])
if test $ac_cv_func_syslog = no; then
  # syslog is not in the default libraries.  See if it's in some other.
  for lib in bsd socket inet; do
    AC_CHECK_LIB([$lib], [syslog], [AC_DEFINE([HAVE_SYSLOG], [1], [FIXME])
      LIBS="$LIBS -l$lib"; break])
  done
fi

AC_CACHE_CHECK([for 3-argument setpriority function],
  [utils_cv_func_setpriority],
  [AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
       [[#include <sys/time.h>
         #include <sys/resource.h>
       ]],
       [[setpriority (0, 0, 0);]])],
    [utils_cv_func_setpriority=yes],
    [utils_cv_func_setpriority=no])])
if test $utils_cv_func_setpriority = no; then
  AC_CHECK_FUNCS([nice])
fi
case $utils_cv_func_setpriority,$ac_cv_func_nice in
*yes*)
  gl_ADD_PROG([optional_bin_progs], [nice])
esac

if test "$cross_compiling" = yes || test -c /dev/stdin <.; then
  AC_DEFINE([DEV_FD_MIGHT_BE_CHR], [1],
    [Define to 1 if /dev/std{in,out,err} and /dev/fd/N, if they exist, might be
     character-special devices whose minor device number is the file
     descriptor number, such as on Solaris.  Leave undefined if they are
     definitely the actual files.  This determination should be done after any
     symbolic links are followed.])
fi

AC_DEFUN([coreutils_DUMMY_1],
[
  AC_REQUIRE([gl_READUTMP])
  if test $ac_cv_header_utmp_h = yes || test $ac_cv_header_utmpx_h = yes; then
    gl_ADD_PROG([optional_bin_progs], [who])
    gl_ADD_PROG([optional_bin_progs], [users])
    gl_ADD_PROG([optional_bin_progs], [pinky])
  fi
])
coreutils_DUMMY_1

AC_SYS_POSIX_TERMIOS()
gl_HEADER_TIOCGWINSZ_NEEDS_SYS_IOCTL

if test $ac_cv_sys_posix_termios = yes; then
  gl_ADD_PROG([optional_bin_progs], [stty])

  AC_MSG_CHECKING([whether termios.h needs _XOPEN_SOURCE])
  AC_CACHE_VAL([su_cv_sys_termios_needs_xopen_source],
  [AC_EGREP_CPP([yes], [#include <termios.h>
#ifdef IUCLC
yes
#endif], su_cv_sys_termios_needs_xopen_source=no,
   AC_EGREP_CPP([yes], [#define _XOPEN_SOURCE
#include <termios.h>
#ifdef IUCLC
yes
#endif], su_cv_sys_termios_needs_xopen_source=yes,
   su_cv_sys_termios_needs_xopen_source=no))])
  AC_MSG_RESULT([$su_cv_sys_termios_needs_xopen_source])
  test $su_cv_sys_termios_needs_xopen_source = yes &&
    AC_DEFINE([TERMIOS_NEEDS_XOPEN_SOURCE], [1], [FIXME])

  AC_MSG_CHECKING([c_line in struct termios])
  AC_CACHE_VAL([su_cv_sys_c_line_in_termios],
  [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#if TERMIOS_NEEDS_XOPEN_SOURCE
                                      #define _XOPEN_SOURCE
                                     #endif
                                     #include <sys/types.h>
                                     #include <termios.h>
                                     struct termios t;
                                     int s = sizeof t.c_line;]])],
    [su_cv_sys_c_line_in_termios=yes],
    [su_cv_sys_c_line_in_termios=no])])
  AC_MSG_RESULT([$su_cv_sys_c_line_in_termios])
  test $su_cv_sys_c_line_in_termios = yes \
    && AC_DEFINE([HAVE_C_LINE], [1], [FIXME])
fi

# FIXME: note that this macro appears above, too.
# I'm leaving it here for now.  This whole thing needs to be modernized...
gl_WINSIZE_IN_PTEM

gl_HEADER_TIOCGWINSZ_IN_TERMIOS_H

if test $gl_cv_sys_tiocgwinsz_needs_termios_h = no && \
   test $gl_cv_sys_tiocgwinsz_needs_sys_ioctl_h = no; then
  AC_MSG_CHECKING([TIOCGWINSZ in sys/pty.h])
  AC_CACHE_VAL([su_cv_sys_tiocgwinsz_in_sys_pty_h],
  [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#ifdef WINSIZE_IN_PTEM
# include <sys/stream.h>
# include <sys/ptem.h>
#endif
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/pty.h>]], [[int x = TIOCGWINSZ;]])],
    [su_cv_sys_tiocgwinsz_in_sys_pty_h=yes],
    [su_cv_sys_tiocgwinsz_in_sys_pty_h=no])])
  AC_MSG_RESULT([$su_cv_sys_tiocgwinsz_in_sys_pty_h])

  test $su_cv_sys_tiocgwinsz_in_sys_pty_h = yes \
      && AC_DEFINE([GWINSZ_IN_SYS_PTY], [1],
                   [Define if your system defines TIOCGWINSZ in sys/pty.h.])
fi

# Build df only if there's a point to it.
if test $gl_cv_list_mounted_fs = yes && test $gl_cv_fs_space = yes; then
  gl_ADD_PROG([optional_bin_progs], [df])
fi

# Build stdbuf only if supported
ac_save_CFLAGS=$CFLAGS
ac_save_LDFLAGS=$LDFLAGS
cu_save_c_werror_flag=$ac_c_werror_flag
AC_LANG_WERROR
# Detect warnings about ignored "constructor" attributes.
gl_WARN_ADD([-Werror], [CFLAGS])
gl_WARN_ADD([-errwarn], [CFLAGS])
# Put this message here, after gl_WARN_ADD's chatter.
AC_MSG_CHECKING([whether this system supports stdbuf])
CFLAGS="-fPIC $CFLAGS"
LDFLAGS="-shared $LDFLAGS"
AC_CACHE_VAL([utils_cv_stdbuf_supported],[
utils_cv_stdbuf_supported=no
# Note we only LINK here rather than RUN to support cross compilation
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([[
    static int stdbuf = 0;

    void __attribute__ ((constructor))
    stdbuf_init (void)
    {
      stdbuf = 1;
    }]],[[
    if (stdbuf != 1)
      return 1;]])
  ],
  [utils_cv_stdbuf_supported=yes])])
AC_MSG_RESULT([$utils_cv_stdbuf_supported])
if test "$utils_cv_stdbuf_supported" = "yes" && test -z "$EXEEXT"; then
  gl_ADD_PROG([optional_bin_progs], [stdbuf])
fi
CFLAGS=$ac_save_CFLAGS
LDFLAGS=$ac_save_LDFLAGS
ac_c_werror_flag=$cu_save_c_werror_flag

# Don't build chcon and runcon if only have gnulib stubs
# Honor --without-selinux to force disable
# and --with-selinux to force enable.
if test "$with_selinux" != no &&
   { test "$with_selinux" = yes ||
     test "$ac_cv_header_selinux_selinux_h" = yes; }; then
  gl_ADD_PROG([optional_bin_progs], [chcon])
  gl_ADD_PROG([optional_bin_progs], [runcon])
fi

# Test compiler support for half precision floating point types (for od)
AC_MSG_CHECKING([IEEE 16 bit floating point])
AC_CACHE_VAL([utils_cv_ieee_16_bit_supported],[
 AC_RUN_IFELSE(
   [AC_LANG_SOURCE([[
     int
     main (void)
     {
        volatile _Float16 hf = 1;
        float f = hf;  /* Ensure compiler can promote to float.  */
        return !(f == 1.0f);
     }
  ]])
  ],[
    utils_cv_ieee_16_bit_supported=yes
  ],[
    utils_cv_ieee_16_bit_supported=no
  ],[
    utils_cv_ieee_16_bit_supported=no
  ])])
AC_MSG_RESULT([$utils_cv_ieee_16_bit_supported])
if test $utils_cv_ieee_16_bit_supported = yes; then
  AC_DEFINE([FLOAT16_SUPPORTED], [1], [IEEE 16 bit float supported])
fi

AC_MSG_CHECKING([Brain 16 bit floating point])
AC_CACHE_VAL([utils_cv_brain_16_bit_supported],[
 AC_RUN_IFELSE(
   [AC_LANG_SOURCE([[
     int
     main (void)
     {
        volatile __bf16 hf = 1;
        float f = hf;  /* Ensure compiler can promote to float.  */
        return !(f == 1.0f);
     }
  ]])
  ],[
    utils_cv_brain_16_bit_supported=yes
  ],[
    utils_cv_brain_16_bit_supported=no
  ],[
    utils_cv_brain_16_bit_supported=no
  ])])
AC_MSG_RESULT([$utils_cv_brain_16_bit_supported])
if test $utils_cv_brain_16_bit_supported = yes; then
  AC_DEFINE([BF16_SUPPORTED], [1], [Brain 16 bit float supported])
fi

ac_save_CFLAGS=$CFLAGS
CFLAGS="-march=armv8-a+crypto $CFLAGS"
AC_MSG_CHECKING([if vmull intrinsic exists])
AC_CACHE_VAL([utils_cv_vmull_intrinsic_exists],[
AC_LINK_IFELSE(
  [AC_LANG_SOURCE([[
    #include <stdio.h>
    #include <sys/auxv.h>
    #include <asm/hwcap.h>
    #include <arm_neon.h>

    int
    main (void)
    {
      uint64x2_t a;
      poly64_t shift64 = vget_lane_p64(vcreate_p64(0xB8BC6765), 0);
      a = vreinterpretq_u64_p128(vmull_p64(shift64, vreinterpretq_p128_u64(a)));
      return (getauxval(AT_HWCAP) & HWCAP_PMULL) > 0;
    }
  ]])
  ],[
    utils_cv_vmull_intrinsic_exists=yes
  ],[
    utils_cv_vmull_intrinsic_exists=no
  ])])
AC_MSG_RESULT([$utils_cv_vmull_intrinsic_exists])
if test $utils_cv_vmull_intrinsic_exists = yes; then
  AC_DEFINE([USE_VMULL_CRC32], [1],
            [CRC32 calculation by vmull hardware instruction enabled])
fi
AM_CONDITIONAL([USE_VMULL_CRC32],
               [test $utils_cv_vmull_intrinsic_exists = yes])
CFLAGS=$ac_save_CFLAGS

ac_save_CFLAGS=$CFLAGS
CFLAGS="-mavx -mpclmul $CFLAGS"
AC_MSG_CHECKING([if pclmul intrinsic exists])
AC_CACHE_VAL([utils_cv_pclmul_intrinsic_exists],[
AC_LINK_IFELSE(
  [AC_LANG_SOURCE([[
    #include <x86intrin.h>

    int
    main (void)
    {
      __m128i a, b;
      a = _mm_clmulepi64_si128 (a, b, 0x00);
      a = _mm_shuffle_epi8 (a, b);
      return __builtin_cpu_supports ("pclmul");
    }
  ]])
  ],[
    utils_cv_pclmul_intrinsic_exists=yes
  ],[
    utils_cv_pclmul_intrinsic_exists=no
  ])])
AC_MSG_RESULT([$utils_cv_pclmul_intrinsic_exists])
if test $utils_cv_pclmul_intrinsic_exists = yes; then
  AC_DEFINE([USE_PCLMUL_CRC32], [1],
            [CRC32 calculation by pclmul hardware instruction enabled])
fi
AM_CONDITIONAL([USE_PCLMUL_CRC32],
               [test $utils_cv_pclmul_intrinsic_exists = yes])
CFLAGS=$ac_save_CFLAGS

ac_save_CFLAGS=$CFLAGS
CFLAGS=" -mavx2 -mvpclmulqdq $CFLAGS"
AC_MSG_CHECKING([if avx2 pclmul intrinsic exists])
AC_CACHE_VAL([utils_cv_avx2_pclmul_intrinsic_exists],[
AC_LINK_IFELSE(
  [AC_LANG_SOURCE([[
    #include <x86intrin.h>

    int
    main (void)
    {
      __m256i a, b;
      a = _mm256_clmulepi64_epi128 (a, b, 0x00);
      a = _mm256_shuffle_epi8 (a, b);
      return __builtin_cpu_supports ("avx2") &&
        __builtin_cpu_supports ("vpclmulqdq");
    }
  ]])
  ],[
    utils_cv_avx2_pclmul_intrinsic_exists=yes
  ],[
    utils_cv_avx2_pclmul_intrinsic_exists=no
  ])])
AC_MSG_RESULT([$utils_cv_avx2_pclmul_intrinsic_exists])
if test $utils_cv_avx2_pclmul_intrinsic_exists = yes; then
  AC_DEFINE([USE_AVX2_CRC32], [1],
            [CRC32 calculation by avx2 hardware instructions enabled])
fi
AM_CONDITIONAL([USE_AVX2_CRC32],
               [test $utils_cv_avx2_pclmul_intrinsic_exists = yes])
CFLAGS=$ac_save_CFLAGS

ac_save_CFLAGS=$CFLAGS
CFLAGS=" -mavx512bw -mavx512f -mvpclmulqdq $CFLAGS"
AC_MSG_CHECKING([if avx512 pclmul intrinsic exists])
AC_CACHE_VAL([utils_cv_avx512_pclmul_intrinsic_exists],[
AC_LINK_IFELSE(
  [AC_LANG_SOURCE([[
    #include <x86intrin.h>

    int
    main (void)
    {
      __m512i a, b;
      a = _mm512_set_epi8 (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      a = _mm512_clmulepi64_epi128 (a, b, 0x00);
      a = _mm512_shuffle_epi8 (a, b);
      return (__builtin_cpu_supports ("avx512bw")
              && __builtin_cpu_supports ("avx512f")
              && __builtin_cpu_supports ("vpclmulqdq"));
    }
  ]])
  ],[
    utils_cv_avx512_pclmul_intrinsic_exists=yes
  ],[
    utils_cv_avx512_pclmul_intrinsic_exists=no
  ])])
AC_MSG_RESULT([$utils_cv_avx512_pclmul_intrinsic_exists])
if test $utils_cv_avx512_pclmul_intrinsic_exists = yes; then
  AC_DEFINE([USE_AVX512_CRC32], [1],
            [CRC32 calculation by avx512 hardware instructions enabled])
fi
AM_CONDITIONAL([USE_AVX512_CRC32],
               [test $utils_cv_avx512_pclmul_intrinsic_exists = yes])
CFLAGS=$ac_save_CFLAGS

CFLAGS="-mavx2 $CFLAGS"
AC_MSG_CHECKING([for avx2 intrinsics])
AC_CACHE_VAL([utils_cv_avx2_intrinsic_exists],[
AC_LINK_IFELSE(
  [AC_LANG_SOURCE([[
    #include <x86intrin.h>

    int
    main (void)
    {
      __m256i matches = _mm256_setzero_si256 ();
      int mask = _mm256_movemask_epi8 (matches);
      int lines = __builtin_popcount (mask);
      return __builtin_cpu_supports ("avx2");
    }
  ]])
  ],[
    utils_cv_avx2_intrinsic_exists=yes
  ],[
    utils_cv_avx2_intrinsic_exists=no
  ])])
AC_MSG_RESULT([$utils_cv_avx2_intrinsic_exists])
if test $utils_cv_avx2_intrinsic_exists = yes; then
  AC_DEFINE([USE_AVX2_WC_LINECOUNT], [1], [Counting lines with AVX2 enabled])
fi
AM_CONDITIONAL([USE_AVX2_WC_LINECOUNT],
               [test $utils_cv_avx2_intrinsic_exists = yes])

CFLAGS=$ac_save_CFLAGS

CFLAGS="-mavx512bw -mavx512f $CFLAGS"
AC_MSG_CHECKING([for avx512 intrinsics])
AC_CACHE_VAL([utils_cv_avx512_intrinsic_exists],[
AC_LINK_IFELSE(
  [AC_LANG_SOURCE([[
    #include <x86intrin.h>

    int
    main (void)
    {
      __m512i matches = _mm512_setzero_si512 ();
      long long mask = _mm512_movepi8_mask (matches);
      int lines = __builtin_popcountll (mask);
      return (__builtin_cpu_supports ("avx512bw")
              && __builtin_cpu_supports ("avx512f"));
    }
  ]])
  ],[
    utils_cv_avx512_intrinsic_exists=yes
  ],[
    utils_cv_avx512_intrinsic_exists=no
  ])])
AC_MSG_RESULT([$utils_cv_avx512_intrinsic_exists])
if test $utils_cv_avx512_intrinsic_exists = yes; then
  AC_DEFINE([USE_AVX512_WC_LINECOUNT], [1],
            [Counting lines with AVX512 enabled])
fi
AM_CONDITIONAL([USE_AVX512_WC_LINECOUNT],
               [test $utils_cv_avx512_intrinsic_exists = yes])

CFLAGS=$ac_save_CFLAGS
############################################################################

dnl Autogenerated by the 'gen-lists-of-programs.sh' auxiliary script.
dnl Issue proper calls to the macros gl_INCLUDE_EXCLUDE_PROG and
dnl gl_ADD_PROG (updating $optional_bin_progs), and generate the list
dnl of coreutils programs to be built only upon explicit user request,
dnl saving that list in the $no_install_progs_default shell variable.
m4_include([m4/cu-progs.m4])

# Now that we know which programs will actually be built, determine
# which optional helper progs should be compiled.
# Note it adding to pkglibexec_PROGRAMS, $(transform) in src/local.mk
# may need to be updated accordingly.
case " $optional_bin_progs " in
  *' stdbuf '*) pkglibexec_PROGRAMS='src/libstdbuf.so';;
  *) pkglibexec_PROGRAMS='';;
esac

man1_MANS=`
  for p in $optional_bin_progs; do
    # Change "ginstall.1" to "install.1".
    test $p = ginstall && p=install
    # Ignore the "[" program, since writing a portable make rule to
    # generate its manpage is not practical.
    dnl Use the autoconf-provided quadrigraph to represent "[",
    dnl otherwise we will incur in dreadful quoting issues.
    test x$p = x'@<:@' && continue
    echo "man/$p.1"
  done`

# Not installed by "make install", but must be built when creating
# a distribution tarball.
EXTRA_MANS=`for p in $no_install_progs_default $gl_no_install_prog; do
              echo man/$p.1
            done`

# Replace all the programs by the single binary and symlinks if specified.
single_binary_progs=
single_binary_libs=
single_binary_deps=
single_binary_install_type=
if test "$gl_single_binary" != no; then
  man1_MANS="$man1_MANS man/coreutils.1"
  # Convert the list to a space separated list
  gl_single_binary_exceptions=`echo $gl_single_binary_exceptions | tr ',' ' '`

  single_binary_progs=`echo $optional_bin_progs`
  optional_bin_progs="coreutils"
  for prog in $gl_single_binary_exceptions; do
    # Fail if requested to exclude a program than was not part of coreutils.
    case " $single_binary_progs " in
      *" $prog "*)
        gl_REMOVE_PROG([single_binary_progs], [$prog]) ;
        gl_ADD_PROG([optional_bin_progs], [$prog]) ;;
      *) AC_MSG_ERROR(['$prog' is not being compiled.]) ;;
    esac
  done

  # single_binary_libs holds the list of libs required by the selected
  # programs, such as for example -lrt.
  single_binary_libs=`
    for p in $single_binary_progs; do
      # Convert '[' to '_'
      test x"$p" = x'@<:@' && p='_'
      printf '$(src_libsinglebin_%s_a_ldadd) ' "$p"
    done`
  # single_binary_deps holds the list of libsinglebin_*.a files that have the
  # compiled code of each selected program in a "library" format.
  single_binary_deps=`
    for p in $single_binary_progs; do
      # Convert '[' to '_'
      test x"$p" = x'@<:@' && p='_'
      printf 'src/libsinglebin_%s.a ' "$p"
    done`
  single_binary_install_type="$gl_single_binary"
fi
AC_SUBST([single_binary_progs], [$single_binary_progs])
AC_SUBST([single_binary_libs], [$single_binary_libs])
AC_SUBST([single_binary_deps], [$single_binary_deps])
AC_SUBST([single_binary_install_type], [$single_binary_install_type])


# The programs built and installed by "make && make install".
# Since this is AC_SUBST'd, Automake won't be able to perform rewrite
# with $(EXEEXT) appending on it, so we have to do it ourselves -- in
# this case, only for $(bin_PROGRAMS).
bin_PROGRAMS=`
  for p in $optional_bin_progs; do echo src/"$p"'$(EXEEXT)'; done`

# Normalize whitespace.
man1_MANS=`echo $man1_MANS`
EXTRA_MANS=`echo $EXTRA_MANS`
bin_PROGRAMS=`echo $bin_PROGRAMS`
pkglibexec_PROGS=`echo $pkglibexec_PROGRAMS`

AC_SUBST([bin_PROGRAMS])        AM_SUBST_NOTMAKE([bin_PROGRAMS])
AC_SUBST([pkglibexec_PROGRAMS]) AM_SUBST_NOTMAKE([pkglibexec_PROGRAMS])
AC_SUBST([man1_MANS])           AM_SUBST_NOTMAKE([man1_MANS])
AC_SUBST([EXTRA_MANS])          AM_SUBST_NOTMAKE([EXTRA_MANS])

AC_SUBST([built_programs], [$optional_bin_progs])

AM_CONDITIONAL([CROSS_COMPILING], [test "$cross_compiling" = yes])
############################################################################

# As long as "grep 'PRI[diouxX]' po/*.pot" reports matches in
# translatable strings, we must use need-formatstring-macros here.
AM_GNU_GETTEXT([external], [need-formatstring-macros])
AM_GNU_GETTEXT_VERSION([0.19.2])
AM_CONDITIONAL([USE_NLS], [test "$USE_NLS" = yes])

# For a test of uniq: it uses the $LOCALE_FR envvar.
gt_LOCALE_FR

AC_CONFIG_FILES(
  Makefile
  po/Makefile.in
  gnulib-tests/Makefile
  )
AC_OUTPUT
