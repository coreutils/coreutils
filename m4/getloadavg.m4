#serial 4

AC_DEFUN(AM_FUNC_GETLOADAVG,
[ac_have_func=no # yes means we've found a way to get the load average.

am_cv_saved_LIBS="$LIBS"

# On HPUX9, an unprivileged user can get load averages through this function.
AC_CHECK_FUNCS(pstat_getdynamic)

# Solaris has libkstat which does not require root.
AC_CHECK_LIB(kstat, kstat_open)
if test $ac_cv_lib_kstat_kstat_open = yes ; then ac_have_func=yes ; fi

# Some systems with -lutil have (and need) -lkvm as well, some do not.
# On Solaris, -lkvm requires nlist from -lelf, so check that first
# to get the right answer into the cache.
# For kstat on solaris, we need libelf to force the definition of SVR4 below.
AC_CHECK_LIB(elf, elf_begin, LIBS="-lelf $LIBS")
if test $ac_have_func = no; then
  AC_CHECK_LIB(kvm, kvm_open, LIBS="-lkvm $LIBS")
  # Check for the 4.4BSD definition of getloadavg.
  AC_CHECK_LIB(util, getloadavg,
    [LIBS="-lutil $LIBS" ac_have_func=yes ac_cv_func_getloadavg_setgid=yes])
fi

if test $ac_have_func = no; then
  # There is a commonly available library for RS/6000 AIX.
  # Since it is not a standard part of AIX, it might be installed locally.
  ac_save_LIBS="$LIBS"
  LIBS="-L/usr/local/lib $LIBS"
  AC_CHECK_LIB(getloadavg, getloadavg,
    LIBS="-lgetloadavg $LIBS", LIBS="$ac_save_LIBS")
fi

# Make sure it is really in the library, if we think we found it.
AC_REPLACE_FUNCS(getloadavg)

if test $ac_cv_func_getloadavg = yes; then
  AC_DEFINE(HAVE_GETLOADAVG)
  ac_have_func=yes
else
  AC_DEFINE(C_GETLOADAVG, 1, [Define if using getloadavg.c.])
  # Figure out what our getloadavg.c needs.
  ac_have_func=no
  AC_CHECK_HEADER(sys/dg_sys_info.h,
  [ac_have_func=yes; AC_DEFINE(DGUX)
  AC_CHECK_LIB(dgc, dg_sys_info)])

  AC_CHECK_HEADER(locale.h)
  AC_CHECK_FUNCS(setlocale)

  # We cannot check for <dwarf.h>, because Solaris 2 does not use dwarf (it
  # uses stabs), but it is still SVR4.  We cannot check for <elf.h> because
  # Irix 4.0.5F has the header but not the library.
  if test $ac_have_func = no && test $ac_cv_lib_elf_elf_begin = yes; then
    ac_have_func=yes; AC_DEFINE(SVR4)
  fi

  if test $ac_have_func = no; then
    AC_CHECK_HEADER(inq_stats/cpustats.h,
    [ac_have_func=yes; AC_DEFINE(UMAX)
    AC_DEFINE(UMAX4_3)])
  fi

  if test $ac_have_func = no; then
    AC_CHECK_HEADER(sys/cpustats.h,
    [ac_have_func=yes; AC_DEFINE(UMAX)])
  fi

  if test $ac_have_func = no; then
    AC_CHECK_HEADERS(mach/mach.h)
  fi

  AC_CHECK_HEADER(nlist.h,
  [AC_DEFINE(NLIST_STRUCT)
  AC_CACHE_CHECK([for n_un in struct nlist], ac_cv_struct_nlist_n_un,
  [AC_TRY_COMPILE([#include <nlist.h>],
  [struct nlist n; n.n_un.n_name = 0;],
  ac_cv_struct_nlist_n_un=yes, ac_cv_struct_nlist_n_un=no)])
  if test $ac_cv_struct_nlist_n_un = yes; then
    AC_DEFINE(NLIST_NAME_UNION)
  fi
  ])dnl
fi # Do not have getloadavg in system libraries.

# Some definitions of getloadavg require that the program be installed setgid.
dnl FIXME Don't hardwire the path of getloadavg.c in the top-level directory.
AC_CACHE_CHECK(whether getloadavg requires setgid,
  ac_cv_func_getloadavg_setgid,
[AC_EGREP_CPP([Yowza Am I SETGID yet],
[#include "$srcdir/lib/getloadavg.c"
#ifdef LDAV_PRIVILEGED
Yowza Am I SETGID yet
#endif],
  ac_cv_func_getloadavg_setgid=yes, ac_cv_func_getloadavg_setgid=no)])
if test $ac_cv_func_getloadavg_setgid = yes; then
  NEED_SETGID=true; AC_DEFINE(GETLOADAVG_PRIVILEGED)
else
  NEED_SETGID=false
fi
AC_SUBST(NEED_SETGID)dnl

if test $ac_cv_func_getloadavg_setgid = yes; then
  AC_CACHE_CHECK(group of /dev/kmem, ac_cv_group_kmem,
changequote(<<, >>)dnl
<<
  # On Solaris, /dev/kmem is a symlink.  Get info on the real file.
  ac_ls_output=`ls -lgL /dev/kmem 2>/dev/null`
  # If we got an error (system does not support symlinks), try without -L.
  test -z "$ac_ls_output" && ac_ls_output=`ls -lg /dev/kmem`
  ac_cv_group_kmem=`echo $ac_ls_output \
    | sed -ne 's/[ 	][ 	]*/ /g;
	       s/^.[sSrwx-]* *[0-9]* *\([^0-9]*\)  *.*/\1/;
	       / /s/.* //;p;'`
>>
changequote([, ])dnl
)
  KMEM_GROUP=$ac_cv_group_kmem
fi
AC_SUBST(KMEM_GROUP)dnl

if test x = "x$am_cv_saved_LIBS"; then
  GETLOADAVG_LIBS="$LIBS"
else
  GETLOADAVG_LIBS=`echo "$LIBS"|sed "s!$am_cv_saved_LIBS!!"`
fi
AC_SUBST(GETLOADAVG_LIBS)dnl
LIBS="$am_cv_saved_LIBS"
])
