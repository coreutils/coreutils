#serial 9

# From fileutils/configure.in

# Try to determine how a program can obtain filesystem usage information.
# If successful, define the appropriate symbol (see fsusage.c) and
# execute ACTION-IF-FOUND.  Otherwise, execute ACTION-IF-NOT-FOUND.
#
# jm_FILE_SYSTEM_USAGE([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])

AC_DEFUN([jm_FILE_SYSTEM_USAGE],
[

echo "checking how to get filesystem space usage..."
ac_fsusage_space=no

# Perform only the link test since it seems there are no variants of the
# statvfs function.  This check is more than just AC_CHECK_FUNCS(statvfs)
# because that got a false positive on SCO OSR5.  Adding the declaration
# of a `struct statvfs' causes this test to fail (as it should) on such
# systems.  That system is reported to work fine with STAT_STATFS4 which
# is what it gets when this test fails.
if test $ac_fsusage_space = no; then
  # SVR4
  AC_CACHE_CHECK([for statvfs function (SVR4)], fu_cv_sys_stat_statvfs,
		 [AC_TRY_LINK([#include <sys/types.h>
#ifdef __GLIBC__
Do not use statvfs on systems with GNU libc, because that function stats
all preceding entries in /proc/mounts, and that makes df hang if even
one of the corresponding file systems is hard-mounted, but not available.
#endif
#include <sys/statvfs.h>],
			      [struct statvfs fsd; statvfs (0, &fsd);],
			      fu_cv_sys_stat_statvfs=yes,
			      fu_cv_sys_stat_statvfs=no)])
  if test $fu_cv_sys_stat_statvfs = yes; then
    ac_fsusage_space=yes
    AC_DEFINE(STAT_STATVFS, 1,
	      [  Define if there is a function named statvfs.  (SVR4)])
  fi
fi

if test $ac_fsusage_space = no; then
  # DEC Alpha running OSF/1
  AC_MSG_CHECKING([for 3-argument statfs function (DEC OSF/1)])
  AC_CACHE_VAL(fu_cv_sys_stat_statfs3_osf1,
  [AC_TRY_RUN([
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
  main ()
  {
    struct statfs fsd;
    fsd.f_fsize = 0;
    exit (statfs (".", &fsd, sizeof (struct statfs)));
  }],
  fu_cv_sys_stat_statfs3_osf1=yes,
  fu_cv_sys_stat_statfs3_osf1=no,
  fu_cv_sys_stat_statfs3_osf1=no)])
  AC_MSG_RESULT($fu_cv_sys_stat_statfs3_osf1)
  if test $fu_cv_sys_stat_statfs3_osf1 = yes; then
    ac_fsusage_space=yes
    AC_DEFINE(STAT_STATFS3_OSF1, 1,
	      [   Define if  statfs takes 3 args.  (DEC Alpha running OSF/1)])
  fi
fi

if test $ac_fsusage_space = no; then
# AIX
  AC_MSG_CHECKING([for two-argument statfs with statfs.bsize dnl
member (AIX, 4.3BSD)])
  AC_CACHE_VAL(fu_cv_sys_stat_statfs2_bsize,
  [AC_TRY_RUN([
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
  main ()
  {
  struct statfs fsd;
  fsd.f_bsize = 0;
  exit (statfs (".", &fsd));
  }],
  fu_cv_sys_stat_statfs2_bsize=yes,
  fu_cv_sys_stat_statfs2_bsize=no,
  fu_cv_sys_stat_statfs2_bsize=no)])
  AC_MSG_RESULT($fu_cv_sys_stat_statfs2_bsize)
  if test $fu_cv_sys_stat_statfs2_bsize = yes; then
    ac_fsusage_space=yes
    AC_DEFINE(STAT_STATFS2_BSIZE, 1,
[  Define if statfs takes 2 args and struct statfs has a field named f_bsize.
   (4.3BSD, SunOS 4, HP-UX, AIX PS/2)])
  fi
fi

if test $ac_fsusage_space = no; then
# SVR3
  AC_MSG_CHECKING([for four-argument statfs (AIX-3.2.5, SVR3)])
  AC_CACHE_VAL(fu_cv_sys_stat_statfs4,
  [AC_TRY_RUN([#include <sys/types.h>
#include <sys/statfs.h>
  main ()
  {
  struct statfs fsd;
  exit (statfs (".", &fsd, sizeof fsd, 0));
  }],
    fu_cv_sys_stat_statfs4=yes,
    fu_cv_sys_stat_statfs4=no,
    fu_cv_sys_stat_statfs4=no)])
  AC_MSG_RESULT($fu_cv_sys_stat_statfs4)
  if test $fu_cv_sys_stat_statfs4 = yes; then
    ac_fsusage_space=yes
    AC_DEFINE(STAT_STATFS4, 1,
	      [  Define if statfs takes 4 args.  (SVR3, Dynix, Irix, Dolphin)])
  fi
fi

if test $ac_fsusage_space = no; then
# 4.4BSD and NetBSD
  AC_MSG_CHECKING([for two-argument statfs with statfs.fsize dnl
member (4.4BSD and NetBSD)])
  AC_CACHE_VAL(fu_cv_sys_stat_statfs2_fsize,
  [AC_TRY_RUN([#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
  main ()
  {
  struct statfs fsd;
  fsd.f_fsize = 0;
  exit (statfs (".", &fsd));
  }],
  fu_cv_sys_stat_statfs2_fsize=yes,
  fu_cv_sys_stat_statfs2_fsize=no,
  fu_cv_sys_stat_statfs2_fsize=no)])
  AC_MSG_RESULT($fu_cv_sys_stat_statfs2_fsize)
  if test $fu_cv_sys_stat_statfs2_fsize = yes; then
    ac_fsusage_space=yes
    AC_DEFINE(STAT_STATFS2_FSIZE, 1,
[  Define if statfs takes 2 args and struct statfs has a field named f_fsize.
   (4.4BSD, NetBSD)])
  fi
fi

if test $ac_fsusage_space = no; then
  # Ultrix
  AC_MSG_CHECKING([for two-argument statfs with struct fs_data (Ultrix)])
  AC_CACHE_VAL(fu_cv_sys_stat_fs_data,
  [AC_TRY_RUN([#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_FS_TYPES_H
#include <sys/fs_types.h>
#endif
  main ()
  {
  struct fs_data fsd;
  /* Ultrix's statfs returns 1 for success,
     0 for not mounted, -1 for failure.  */
  exit (statfs (".", &fsd) != 1);
  }],
  fu_cv_sys_stat_fs_data=yes,
  fu_cv_sys_stat_fs_data=no,
  fu_cv_sys_stat_fs_data=no)])
  AC_MSG_RESULT($fu_cv_sys_stat_fs_data)
  if test $fu_cv_sys_stat_fs_data = yes; then
    ac_fsusage_space=yes
    AC_DEFINE(STAT_STATFS2_FS_DATA, 1,
[  Define if statfs takes 2 args and the second argument has
   type struct fs_data.  (Ultrix)])
  fi
fi

if test $ac_fsusage_space = no; then
  # SVR2
  AC_TRY_CPP([#include <sys/filsys.h>
    ],
    AC_DEFINE(STAT_READ_FILSYS, 1,
      [Define if there is no specific function for reading filesystems usage
       information and you have the <sys/filsys.h> header file.  (SVR2)])
    ac_fsusage_space=yes)
fi

AS_IF([test $ac_fsusage_space = yes], [$1], [$2])

])
