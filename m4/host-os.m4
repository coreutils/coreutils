#serial 2

dnl From Paul Eggert.

# Define HOST_OPERATING_SYSTEM to a name for the host operating system.
AC_DEFUN([UTILS_HOST_OS],
[
  AC_CACHE_CHECK([host operating system],
    utils_cv_host_operating_system,

    [[case $host_os in

       # These operating system names do not use the default heuristic below.
       # They are in reverse order, so that more-specific prefixes come first.
       winnt*)		os='Windows NT';;
       vos*)		os='VOS';;
       sysv*)		os='Unix System V';;
       superux*)	os='SUPER-UX';;
       sunos*)		os='SunOS';;
       stop*)		os='STOP';;
       sco*)		os='SCO Unix';;
       riscos*)		os='RISC OS';;
       riscix*)		os='RISCiX';;
       qnx*)		os='QNX';;
       pw32*)		os='PW32';;
       ptx*)		os='ptx';;
       plan9*)		os='Plan 9';;
       osf*)		os='Tru64';;
       os2*)		os='OS/2';;
       openbsd*)	os='OpenBSD';;
       nsk*)		os='NonStop Kernel';;
       nonstopux*)	os='NonStop-UX';;
       netbsd*)		os='NetBSD';;
       msdosdjgpp*)	os='DJGPP';;
       mpeix*)		os='MPE/iX';;
       mint*)		os='MiNT';;
       mingw*)		os='MinGW';;
       lynxos*)		os='LynxOS';;
       linux*)		os='GNU/Linux';;
       hpux*)		os='HP-UX';;
       hiux*)		os='HI-UX';;
       gnu*)		os='GNU';;
       freebsd*-gnu*)	os='GNU/FreeBSD';;
       freebsd*)	os='FreeBSD';;
       dgux*)		os='DG/UX';;
       bsdi*)		os='BSD/OS';;
       bsd*)		os='BSD';;
       beos*)		os='BeOS';;
       aux*)		os='A/UX';;
       atheos*)		os='AtheOS';;
       amigaos*)	os='Amiga OS';;
       aix*)		os='AIX';;

       # The default heuristic takes the initial alphabetic string
       # from $host_os, but capitalizes its first letter.
       [A-Za-z]*)
	 os=`
	   expr "X$host_os" : 'X\([A-Za-z]\)' | tr '[a-z]' '[A-Z]'
	 ``
	   expr "X$host_os" : 'X.\([A-Za-z]*\)'
	 `
	 ;;

       # If $host_os does not start with an alphabetic string, use it unchanged.
       *)
	 os=$host_os;;
     esac
     utils_cv_host_operating_system=$os]])
  AC_DEFINE_UNQUOTED(HOST_OPERATING_SYSTEM,
    "$utils_cv_host_operating_system",
    [The host operating system.])
])
