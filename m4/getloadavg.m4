#serial 9

# A replacement for autoconf's macro by the same name.  This version
# accepts an optional argument specifying the name of the $srcdir-relative
# directory in which the file getloadavg.c may be found.  It is unusual
# (but justified, imho) that this file is required at ./configure time.

undefine([AC_FUNC_GETLOADAVG])

# AC_FUNC_GETLOADAVG
# ------------------
AC_DEFUN([AC_FUNC_GETLOADAVG],
[ac_have_func=no # yes means we've found a way to get the load average.

AC_CHECK_HEADERS(locale.h unistd.h mach/mach.h fcntl.h)
AC_CHECK_FUNCS(setlocale)

# By default, expect to find getloadavg.c in $srcdir/.
ac_lib_dir_getloadavg=$srcdir
# But if there's an argument, DIR, expect to find getloadavg.c in $srcdir/DIR.
m4_ifval([$1], [ac_lib_dir_getloadavg=$srcdir/$1])
# Make sure getloadavg.c is where it belongs, at ./configure-time.
test -f $ac_lib_dir_getloadavg/getloadavg.c \
  || AC_MSG_ERROR([getloadavg.c is not in $ac_lib_dir_getloadavg])
# FIXME: Add an autoconf-time test, too?

ac_save_LIBS=$LIBS

# Check for getloadavg, but be sure not to touch the cache variable.
(AC_CHECK_FUNC(getloadavg, exit 0, exit 1)) && ac_have_func=yes

# On HPUX9, an unprivileged user can get load averages through this function.
AC_CHECK_FUNCS(pstat_getdynamic)

# Solaris has libkstat which does not require root.
AC_CHECK_LIB(kstat, kstat_open)
test $ac_cv_lib_kstat_kstat_open = yes && ac_have_func=yes

# Some systems with -lutil have (and need) -lkvm as well, some do not.
# On Solaris, -lkvm requires nlist from -lelf, so check that first
# to get the right answer into the cache.
# For kstat on solaris, we need libelf to force the definition of SVR4 below.
if test $ac_have_func = no; then
  AC_CHECK_LIB(elf, elf_begin, LIBS="-lelf $LIBS")
fi
if test $ac_have_func = no; then
  AC_CHECK_LIB(kvm, kvm_open, LIBS="-lkvm $LIBS")
  # Check for the 4.4BSD definition of getloadavg.
  AC_CHECK_LIB(util, getloadavg,
    [LIBS="-lutil $LIBS" ac_have_func=yes ac_cv_func_getloadavg_setgid=yes])
fi

if test $ac_have_func = no; then
  # There is a commonly available library for RS/6000 AIX.
  # Since it is not a standard part of AIX, it might be installed locally.
  ac_getloadavg_LIBS=$LIBS
  LIBS="-L/usr/local/lib $LIBS"
  AC_CHECK_LIB(getloadavg, getloadavg,
               [LIBS="-lgetloadavg $LIBS"], [LIBS=$ac_getloadavg_LIBS])
fi

# Make sure it is really in the library, if we think we found it,
# otherwise set up the replacement function.
AC_CHECK_FUNCS(getloadavg, [],
               [_AC_LIBOBJ_GETLOADAVG])

# Some definitions of getloadavg require that the program be installed setgid.
AC_CACHE_CHECK(whether getloadavg requires setgid,
               ac_cv_func_getloadavg_setgid,
[AC_EGREP_CPP([Yowza Am I SETGID yet],
[#include "$ac_lib_dir_getloadavg/getloadavg.c"
#ifdef LDAV_PRIVILEGED
Yowza Am I SETGID yet
@%:@endif],
              ac_cv_func_getloadavg_setgid=yes,
              ac_cv_func_getloadavg_setgid=no)])
if test $ac_cv_func_getloadavg_setgid = yes; then
  NEED_SETGID=true
  AC_DEFINE(GETLOADAVG_PRIVILEGED, 1,
            [Define if the `getloadavg' function needs to be run setuid
             or setgid.])
else
  NEED_SETGID=false
fi
AC_SUBST(NEED_SETGID)dnl

if test $ac_cv_func_getloadavg_setgid = yes; then
  AC_CACHE_CHECK(group of /dev/kmem, ac_cv_group_kmem,
[ # On Solaris, /dev/kmem is a symlink.  Get info on the real file.
  ac_ls_output=`ls -lgL /dev/kmem 2>/dev/null`
  # If we got an error (system does not support symlinks), try without -L.
  test -z "$ac_ls_output" && ac_ls_output=`ls -lg /dev/kmem`
  ac_cv_group_kmem=`echo $ac_ls_output \
    | sed -ne ['s/[ 	][ 	]*/ /g;
	       s/^.[sSrwx-]* *[0-9]* *\([^0-9]*\)  *.*/\1/;
	       / /s/.* //;p;']`
])
  AC_SUBST(KMEM_GROUP, $ac_cv_group_kmem)dnl
fi
if test "x$ac_save_LIBS" = x; then
  GETLOADAVG_LIBS=$LIBS
else
  GETLOADAVG_LIBS=`echo "$LIBS" | sed "s!$ac_save_LIBS!!"`
fi
LIBS=$ac_save_LIBS

AC_SUBST(GETLOADAVG_LIBS)dnl
])# AC_FUNC_GETLOADAVG
