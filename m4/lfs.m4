#serial 3

dnl The problem is that the default compilation flags in Solaris 2.6 won't
dnl let programs access large files;  you need to tell the compiler that
dnl you actually want your programs to work on large files.  For more
dnl details about this brain damage please see:
dnl http://www.sas.com/standards/large.file/x_open.20Mar96.html

dnl Written by Paul Eggert <eggert@twinsun.com>.

dnl Internal subroutine of AC_LFS.
dnl AC_LFS_FLAGS(FLAGSNAME)
AC_DEFUN(AC_LFS_FLAGS,
  [AC_CACHE_CHECK([for $1 value to request large file support], ac_cv_lfs_$1,
     [ac_cv_lfs_$1=`($GETCONF LFS_$1) 2>/dev/null` || {
	ac_cv_lfs_$1=no
	ifelse($1, CFLAGS,
	  [case "$host_os" in
	     # IRIX 6.2 and later
	     irix6.[[2-9]]* | irix6.1[[0-9]]* | irix[[7-9]].* | irix[[1-9]][[0-9]]*)
	       if test "$GCC" != yes; then
		 ac_cv_lfs_CFLAGS=-n32
	       fi
	       ac_save_CC="$CC"
	       CC="$ac_save_CC $ac_cv_lfs_CFLAGS"
	       AC_TRY_LINK(, , , ac_cv_lfs_CFLAGS=no)
	       CC="$ac_save_CC"
	   esac])
      }])])

dnl Internal subroutine of AC_LFS.
dnl AC_LFS_SPACE_APPEND(VAR, VAL)
AC_DEFUN(AC_LFS_SPACE_APPEND,
  [case $2 in
     no) ;;
     ?*)
       case "[$]$1" in
	 '') $1=$2 ;;
	 *) $1=[$]$1' '$2 ;;
       esac ;;
   esac])

dnl Internal subroutine of AC_LFS.
dnl AC_LFS_MACRO_VALUE(C-MACRO, CACHE-VAR, COMMENT, CODE-TO-SET-DEFAULT)
AC_DEFUN(AC_LFS_MACRO_VALUE,
  [AC_CACHE_CHECK([for $1], $2,
     [[$2=no
       $4
       for ac_flag in $ac_cv_lfs_CFLAGS no; do
	 case "$ac_flag" in
	   -D$1)
	     $2=1 ;;
	   -D$1=*)
	     $2=`expr " $ac_flag" : '[^=]*=\(.*\)'` ;;
	 esac
       done]])
   if test "[$]$2" != no; then
     AC_DEFINE_UNQUOTED([$1], [$]$2, [$3])
   fi])

AC_DEFUN(AC_LFS,
  [AC_REQUIRE([AC_CANONICAL_HOST])
   AC_ARG_ENABLE(lfs,
     [  --disable-lfs           omit Large File Support])
   if test "$enable_lfs" != no; then
     AC_CHECK_TOOL(GETCONF, getconf)
     AC_LFS_FLAGS(CFLAGS)
     AC_LFS_FLAGS(LDFLAGS)
     AC_LFS_FLAGS(LIBS)
     for ac_flag in $ac_cv_lfs_CFLAGS no; do
       case "$ac_flag" in
	 no) ;;
	 -D_FILE_OFFSET_BITS=*) ;;
	 -D_LARGEFILE_SOURCE | -D_LARGEFILE_SOURCE=*) ;;
	 -D_LARGE_FILES | -D_LARGE_FILES=*) ;;
	 -[[DI]]?*)
	   AC_LFS_SPACE_APPEND(CPPFLAGS, "$ac_flag") ;;
	 *)
	   AC_LFS_SPACE_APPEND(CFLAGS, "$ac_flag") ;;
       esac
     done
     AC_LFS_SPACE_APPEND(LDFLAGS, "$ac_cv_lfs_LDFLAGS")
     AC_LFS_SPACE_APPEND(LIBS, "$ac_cv_lfs_LIBS")
     AC_LFS_MACRO_VALUE(_FILE_OFFSET_BITS, ac_cv_file_offset_bits,
       [Number of bits in a file offset, on hosts where this is settable. ],
       [case "$host_os" in
	  # HP-UX 10.20 and later
	  hpux10.[2-9][0-9]* | hpux1[1-9]* | hpux[2-9][0-9]*)
	    ac_cv_file_offset_bits=64 ;;
	esac])
     AC_LFS_MACRO_VALUE(_LARGEFILE_SOURCE, ac_cv_largefile_source,
       [Define to make fseeko etc. visible, on some hosts. ],
       [case "$host_os" in
	  # HP-UX 10.20 and later
	  hpux10.[2-9][0-9]* | hpux1[1-9]* | hpux[2-9][0-9]*)
	    ac_cv_largefile_source=1 ;;
	esac])
     AC_LFS_MACRO_VALUE(_LARGE_FILES, ac_cv_large_files,
       [Define for large files, on AIX-style hosts. ],
       [case "$host_os" in
	  # AIX 4.2 and later
	  aix4.[2-9]* | aix4.1[0-9]* | aix[5-9].* | aix[1-9][0-9]*)
	    ac_cv_large_files=1 ;;
	esac])
   fi
  ])
