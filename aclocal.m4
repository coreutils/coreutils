# generated automatically by aclocal 1.7.3 -*- Autoconf -*-

# Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002
# Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# Do all the work for Automake.                            -*- Autoconf -*-

# This macro actually does too much some checks are only needed if
# your package does certain things.  But this isn't really a big deal.

# Copyright 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
# Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 9

# There are a few dirty hacks below to avoid letting `AC_PROG_CC' be
# written in clear, in which case automake, when reading aclocal.m4,
# will think it sees a *use*, and therefore will trigger all it's
# C support machinery.  Also note that it means that autoscan, seeing
# CC etc. in the Makefile, will ask for an AC_PROG_CC use...


AC_PREREQ([2.54])

# Autoconf 2.50 wants to disallow AM_ names.  We explicitly allow
# the ones we care about.
m4_pattern_allow([^AM_[A-Z]+FLAGS$])dnl

# AM_INIT_AUTOMAKE(PACKAGE, VERSION, [NO-DEFINE])
# AM_INIT_AUTOMAKE([OPTIONS])
# -----------------------------------------------
# The call with PACKAGE and VERSION arguments is the old style
# call (pre autoconf-2.50), which is being phased out.  PACKAGE
# and VERSION should now be passed to AC_INIT and removed from
# the call to AM_INIT_AUTOMAKE.
# We support both call styles for the transition.  After
# the next Automake release, Autoconf can make the AC_INIT
# arguments mandatory, and then we can depend on a new Autoconf
# release and drop the old call support.
AC_DEFUN([AM_INIT_AUTOMAKE],
[AC_REQUIRE([AM_SET_CURRENT_AUTOMAKE_VERSION])dnl
 AC_REQUIRE([AC_PROG_INSTALL])dnl
# test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" &&
   test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi

# test whether we have cygpath
if test -z "$CYGPATH_W"; then
  if (cygpath --version) >/dev/null 2>/dev/null; then
    CYGPATH_W='cygpath -w'
  else
    CYGPATH_W=echo
  fi
fi
AC_SUBST([CYGPATH_W])

# Define the identity of the package.
dnl Distinguish between old-style and new-style calls.
m4_ifval([$2],
[m4_ifval([$3], [_AM_SET_OPTION([no-define])])dnl
 AC_SUBST([PACKAGE], [$1])dnl
 AC_SUBST([VERSION], [$2])],
[_AM_SET_OPTIONS([$1])dnl
 AC_SUBST([PACKAGE], ['AC_PACKAGE_TARNAME'])dnl
 AC_SUBST([VERSION], ['AC_PACKAGE_VERSION'])])dnl

_AM_IF_OPTION([no-define],,
[AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
 AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package])])dnl

# Some tools Automake needs.
AC_REQUIRE([AM_SANITY_CHECK])dnl
AC_REQUIRE([AC_ARG_PROGRAM])dnl
AM_MISSING_PROG(ACLOCAL, aclocal-${am__api_version})
AM_MISSING_PROG(AUTOCONF, autoconf)
AM_MISSING_PROG(AUTOMAKE, automake-${am__api_version})
AM_MISSING_PROG(AUTOHEADER, autoheader)
AM_MISSING_PROG(MAKEINFO, makeinfo)
AM_MISSING_PROG(AMTAR, tar)
AM_PROG_INSTALL_SH
AM_PROG_INSTALL_STRIP
# We need awk for the "check" target.  The system "awk" is bad on
# some platforms.
AC_REQUIRE([AC_PROG_AWK])dnl
AC_REQUIRE([AC_PROG_MAKE_SET])dnl
AC_REQUIRE([AM_SET_LEADING_DOT])dnl

_AM_IF_OPTION([no-dependencies],,
[AC_PROVIDE_IFELSE([AC_PROG_CC],
                  [_AM_DEPENDENCIES(CC)],
                  [define([AC_PROG_CC],
                          defn([AC_PROG_CC])[_AM_DEPENDENCIES(CC)])])dnl
AC_PROVIDE_IFELSE([AC_PROG_CXX],
                  [_AM_DEPENDENCIES(CXX)],
                  [define([AC_PROG_CXX],
                          defn([AC_PROG_CXX])[_AM_DEPENDENCIES(CXX)])])dnl
])
])


# When config.status generates a header, we must update the stamp-h file.
# This file resides in the same directory as the config header
# that is generated.  The stamp files are numbered to have different names.

# Autoconf calls _AC_AM_CONFIG_HEADER_HOOK (when defined) in the
# loop where config.status creates the headers, so we can generate
# our stamp files there.
AC_DEFUN([_AC_AM_CONFIG_HEADER_HOOK],
[# Compute $1's index in $config_headers.
_am_stamp_count=1
for _am_header in $config_headers :; do
  case $_am_header in
    $1 | $1:* )
      break ;;
    * )
      _am_stamp_count=`expr $_am_stamp_count + 1` ;;
  esac
done
echo "timestamp for $1" >`AS_DIRNAME([$1])`/stamp-h[]$_am_stamp_count])

# Copyright 2002  Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA

# AM_AUTOMAKE_VERSION(VERSION)
# ----------------------------
# Automake X.Y traces this macro to ensure aclocal.m4 has been
# generated from the m4 files accompanying Automake X.Y.
AC_DEFUN([AM_AUTOMAKE_VERSION],[am__api_version="1.7"])

# AM_SET_CURRENT_AUTOMAKE_VERSION
# -------------------------------
# Call AM_AUTOMAKE_VERSION so it can be traced.
# This function is AC_REQUIREd by AC_INIT_AUTOMAKE.
AC_DEFUN([AM_SET_CURRENT_AUTOMAKE_VERSION],
	 [AM_AUTOMAKE_VERSION([1.7.3])])

# Helper functions for option handling.                    -*- Autoconf -*-

# Copyright 2001, 2002  Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 2

# _AM_MANGLE_OPTION(NAME)
# -----------------------
AC_DEFUN([_AM_MANGLE_OPTION],
[[_AM_OPTION_]m4_bpatsubst($1, [[^a-zA-Z0-9_]], [_])])

# _AM_SET_OPTION(NAME)
# ------------------------------
# Set option NAME.  Presently that only means defining a flag for this option.
AC_DEFUN([_AM_SET_OPTION],
[m4_define(_AM_MANGLE_OPTION([$1]), 1)])

# _AM_SET_OPTIONS(OPTIONS)
# ----------------------------------
# OPTIONS is a space-separated list of Automake options.
AC_DEFUN([_AM_SET_OPTIONS],
[AC_FOREACH([_AM_Option], [$1], [_AM_SET_OPTION(_AM_Option)])])

# _AM_IF_OPTION(OPTION, IF-SET, [IF-NOT-SET])
# -------------------------------------------
# Execute IF-SET if OPTION is set, IF-NOT-SET otherwise.
AC_DEFUN([_AM_IF_OPTION],
[m4_ifset(_AM_MANGLE_OPTION([$1]), [$2], [$3])])

#
# Check to make sure that the build environment is sane.
#

# Copyright 1996, 1997, 2000, 2001 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 3

# AM_SANITY_CHECK
# ---------------
AC_DEFUN([AM_SANITY_CHECK],
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftest.file
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftest.file 2> /dev/null`
   if test "$[*]" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftest.file`
   fi
   rm -f conftest.file
   if test "$[*]" != "X $srcdir/configure conftest.file" \
      && test "$[*]" != "X conftest.file $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "$[2]" = conftest.file
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
AC_MSG_RESULT(yes)])

#  -*- Autoconf -*-


# Copyright 1997, 1999, 2000, 2001 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 3

# AM_MISSING_PROG(NAME, PROGRAM)
# ------------------------------
AC_DEFUN([AM_MISSING_PROG],
[AC_REQUIRE([AM_MISSING_HAS_RUN])
$1=${$1-"${am_missing_run}$2"}
AC_SUBST($1)])


# AM_MISSING_HAS_RUN
# ------------------
# Define MISSING if not defined so far and test if it supports --run.
# If it does, set am_missing_run to use it, otherwise, to nothing.
AC_DEFUN([AM_MISSING_HAS_RUN],
[AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
test x"${MISSING+set}" = xset || MISSING="\${SHELL} $am_aux_dir/missing"
# Use eval to expand $SHELL
if eval "$MISSING --run true"; then
  am_missing_run="$MISSING --run "
else
  am_missing_run=
  AC_MSG_WARN([`missing' script is too old or missing])
fi
])

# AM_AUX_DIR_EXPAND

# Copyright 2001 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# For projects using AC_CONFIG_AUX_DIR([foo]), Autoconf sets
# $ac_aux_dir to `$srcdir/foo'.  In other projects, it is set to
# `$srcdir', `$srcdir/..', or `$srcdir/../..'.
#
# Of course, Automake must honor this variable whenever it calls a
# tool from the auxiliary directory.  The problem is that $srcdir (and
# therefore $ac_aux_dir as well) can be either absolute or relative,
# depending on how configure is run.  This is pretty annoying, since
# it makes $ac_aux_dir quite unusable in subdirectories: in the top
# source directory, any form will work fine, but in subdirectories a
# relative path needs to be adjusted first.
#
# $ac_aux_dir/missing
#    fails when called from a subdirectory if $ac_aux_dir is relative
# $top_srcdir/$ac_aux_dir/missing
#    fails if $ac_aux_dir is absolute,
#    fails when called from a subdirectory in a VPATH build with
#          a relative $ac_aux_dir
#
# The reason of the latter failure is that $top_srcdir and $ac_aux_dir
# are both prefixed by $srcdir.  In an in-source build this is usually
# harmless because $srcdir is `.', but things will broke when you
# start a VPATH build or use an absolute $srcdir.
#
# So we could use something similar to $top_srcdir/$ac_aux_dir/missing,
# iff we strip the leading $srcdir from $ac_aux_dir.  That would be:
#   am_aux_dir='\$(top_srcdir)/'`expr "$ac_aux_dir" : "$srcdir//*\(.*\)"`
# and then we would define $MISSING as
#   MISSING="\${SHELL} $am_aux_dir/missing"
# This will work as long as MISSING is not called from configure, because
# unfortunately $(top_srcdir) has no meaning in configure.
# However there are other variables, like CC, which are often used in
# configure, and could therefore not use this "fixed" $ac_aux_dir.
#
# Another solution, used here, is to always expand $ac_aux_dir to an
# absolute PATH.  The drawback is that using absolute paths prevent a
# configured tree to be moved without reconfiguration.

# Rely on autoconf to set up CDPATH properly.
AC_PREREQ([2.50])

AC_DEFUN([AM_AUX_DIR_EXPAND], [
# expand $ac_aux_dir to an absolute path
am_aux_dir=`cd $ac_aux_dir && pwd`
])

# AM_PROG_INSTALL_SH
# ------------------
# Define $install_sh.

# Copyright 2001 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

AC_DEFUN([AM_PROG_INSTALL_SH],
[AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
install_sh=${install_sh-"$am_aux_dir/install-sh"}
AC_SUBST(install_sh)])

# AM_PROG_INSTALL_STRIP

# Copyright 2001 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# One issue with vendor `install' (even GNU) is that you can't
# specify the program used to strip binaries.  This is especially
# annoying in cross-compiling environments, where the build's strip
# is unlikely to handle the host's binaries.
# Fortunately install-sh will honor a STRIPPROG variable, so we
# always use install-sh in `make install-strip', and initialize
# STRIPPROG with the value of the STRIP variable (set by the user).
AC_DEFUN([AM_PROG_INSTALL_STRIP],
[AC_REQUIRE([AM_PROG_INSTALL_SH])dnl
# Installed binaries are usually stripped using `strip' when the user
# run `make install-strip'.  However `strip' might not be the right
# tool to use in cross-compilation environments, therefore Automake
# will honor the `STRIP' environment variable to overrule this program.
dnl Don't test for $cross_compiling = yes, because it might be `maybe'.
if test "$cross_compiling" != no; then
  AC_CHECK_TOOL([STRIP], [strip], :)
fi
INSTALL_STRIP_PROGRAM="\${SHELL} \$(install_sh) -c -s"
AC_SUBST([INSTALL_STRIP_PROGRAM])])

#                                                          -*- Autoconf -*-
# Copyright (C) 2003  Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 1

# Check whether the underlying file-system supports filenames
# with a leading dot.  For instance MS-DOS doesn't.
AC_DEFUN([AM_SET_LEADING_DOT],
[rm -rf .tst 2>/dev/null
mkdir .tst 2>/dev/null
if test -d .tst; then
  am__leading_dot=.
else
  am__leading_dot=_
fi
rmdir .tst 2>/dev/null
AC_SUBST([am__leading_dot])])

# serial 5						-*- Autoconf -*-

# Copyright (C) 1999, 2000, 2001, 2002, 2003  Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.


# There are a few dirty hacks below to avoid letting `AC_PROG_CC' be
# written in clear, in which case automake, when reading aclocal.m4,
# will think it sees a *use*, and therefore will trigger all it's
# C support machinery.  Also note that it means that autoscan, seeing
# CC etc. in the Makefile, will ask for an AC_PROG_CC use...



# _AM_DEPENDENCIES(NAME)
# ----------------------
# See how the compiler implements dependency checking.
# NAME is "CC", "CXX", "GCJ", or "OBJC".
# We try a few techniques and use that to set a single cache variable.
#
# We don't AC_REQUIRE the corresponding AC_PROG_CC since the latter was
# modified to invoke _AM_DEPENDENCIES(CC); we would have a circular
# dependency, and given that the user is not expected to run this macro,
# just rely on AC_PROG_CC.
AC_DEFUN([_AM_DEPENDENCIES],
[AC_REQUIRE([AM_SET_DEPDIR])dnl
AC_REQUIRE([AM_OUTPUT_DEPENDENCY_COMMANDS])dnl
AC_REQUIRE([AM_MAKE_INCLUDE])dnl
AC_REQUIRE([AM_DEP_TRACK])dnl

ifelse([$1], CC,   [depcc="$CC"   am_compiler_list=],
       [$1], CXX,  [depcc="$CXX"  am_compiler_list=],
       [$1], OBJC, [depcc="$OBJC" am_compiler_list='gcc3 gcc'],
       [$1], GCJ,  [depcc="$GCJ"  am_compiler_list='gcc3 gcc'],
                   [depcc="$$1"   am_compiler_list=])

AC_CACHE_CHECK([dependency style of $depcc],
               [am_cv_$1_dependencies_compiler_type],
[if test -z "$AMDEP_TRUE" && test -f "$am_depcomp"; then
  # We make a subdir and do the tests there.  Otherwise we can end up
  # making bogus files that we don't know about and never remove.  For
  # instance it was reported that on HP-UX the gcc test will end up
  # making a dummy file named `D' -- because `-MD' means `put the output
  # in D'.
  mkdir conftest.dir
  # Copy depcomp to subdir because otherwise we won't find it if we're
  # using a relative directory.
  cp "$am_depcomp" conftest.dir
  cd conftest.dir

  am_cv_$1_dependencies_compiler_type=none
  if test "$am_compiler_list" = ""; then
     am_compiler_list=`sed -n ['s/^#*\([a-zA-Z0-9]*\))$/\1/p'] < ./depcomp`
  fi
  for depmode in $am_compiler_list; do
    # We need to recreate these files for each test, as the compiler may
    # overwrite some of them when testing with obscure command lines.
    # This happens at least with the AIX C compiler.
    echo '#include "conftest.h"' > conftest.c
    echo 'int i;' > conftest.h
    echo "${am__include} ${am__quote}conftest.Po${am__quote}" > confmf

    case $depmode in
    nosideeffect)
      # after this tag, mechanisms are not by side-effect, so they'll
      # only be used when explicitly requested
      if test "x$enable_dependency_tracking" = xyes; then
	continue
      else
	break
      fi
      ;;
    none) break ;;
    esac
    # We check with `-c' and `-o' for the sake of the "dashmstdout"
    # mode.  It turns out that the SunPro C++ compiler does not properly
    # handle `-M -o', and we need to detect this.
    if depmode=$depmode \
       source=conftest.c object=conftest.o \
       depfile=conftest.Po tmpdepfile=conftest.TPo \
       $SHELL ./depcomp $depcc -c -o conftest.o conftest.c \
         >/dev/null 2>conftest.err &&
       grep conftest.h conftest.Po > /dev/null 2>&1 &&
       ${MAKE-make} -s -f confmf > /dev/null 2>&1; then
      # icc doesn't choke on unknown options, it will just issue warnings
      # (even with -Werror).  So we grep stderr for any message
      # that says an option was ignored.
      if grep 'ignoring option' conftest.err >/dev/null 2>&1; then :; else
        am_cv_$1_dependencies_compiler_type=$depmode
        break
      fi
    fi
  done

  cd ..
  rm -rf conftest.dir
else
  am_cv_$1_dependencies_compiler_type=none
fi
])
AC_SUBST([$1DEPMODE], [depmode=$am_cv_$1_dependencies_compiler_type])
AM_CONDITIONAL([am__fastdep$1], [
  test "x$enable_dependency_tracking" != xno \
  && test "$am_cv_$1_dependencies_compiler_type" = gcc3])
])


# AM_SET_DEPDIR
# -------------
# Choose a directory name for dependency files.
# This macro is AC_REQUIREd in _AM_DEPENDENCIES
AC_DEFUN([AM_SET_DEPDIR],
[AC_REQUIRE([AM_SET_LEADING_DOT])dnl
AC_SUBST([DEPDIR], ["${am__leading_dot}deps"])dnl
])


# AM_DEP_TRACK
# ------------
AC_DEFUN([AM_DEP_TRACK],
[AC_ARG_ENABLE(dependency-tracking,
[  --disable-dependency-tracking Speeds up one-time builds
  --enable-dependency-tracking  Do not reject slow dependency extractors])
if test "x$enable_dependency_tracking" != xno; then
  am_depcomp="$ac_aux_dir/depcomp"
  AMDEPBACKSLASH='\'
fi
AM_CONDITIONAL([AMDEP], [test "x$enable_dependency_tracking" != xno])
AC_SUBST([AMDEPBACKSLASH])
])

# Generate code to set up dependency tracking.   -*- Autoconf -*-

# Copyright 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

#serial 2

# _AM_OUTPUT_DEPENDENCY_COMMANDS
# ------------------------------
AC_DEFUN([_AM_OUTPUT_DEPENDENCY_COMMANDS],
[for mf in $CONFIG_FILES; do
  # Strip MF so we end up with the name of the file.
  mf=`echo "$mf" | sed -e 's/:.*$//'`
  # Check whether this is an Automake generated Makefile or not.
  # We used to match only the files named `Makefile.in', but
  # some people rename them; so instead we look at the file content.
  # Grep'ing the first line is not enough: some people post-process
  # each Makefile.in and add a new line on top of each file to say so.
  # So let's grep whole file.
  if grep '^#.*generated by automake' $mf > /dev/null 2>&1; then
    dirpart=`AS_DIRNAME("$mf")`
  else
    continue
  fi
  grep '^DEP_FILES *= *[[^ @%:@]]' < "$mf" > /dev/null || continue
  # Extract the definition of DEP_FILES from the Makefile without
  # running `make'.
  DEPDIR=`sed -n -e '/^DEPDIR = / s///p' < "$mf"`
  test -z "$DEPDIR" && continue
  # When using ansi2knr, U may be empty or an underscore; expand it
  U=`sed -n -e '/^U = / s///p' < "$mf"`
  test -d "$dirpart/$DEPDIR" || mkdir "$dirpart/$DEPDIR"
  # We invoke sed twice because it is the simplest approach to
  # changing $(DEPDIR) to its actual value in the expansion.
  for file in `sed -n -e '
    /^DEP_FILES = .*\\\\$/ {
      s/^DEP_FILES = //
      :loop
	s/\\\\$//
	p
	n
	/\\\\$/ b loop
      p
    }
    /^DEP_FILES = / s/^DEP_FILES = //p' < "$mf" | \
       sed -e 's/\$(DEPDIR)/'"$DEPDIR"'/g' -e 's/\$U/'"$U"'/g'`; do
    # Make sure the directory exists.
    test -f "$dirpart/$file" && continue
    fdir=`AS_DIRNAME(["$file"])`
    AS_MKDIR_P([$dirpart/$fdir])
    # echo "creating $dirpart/$file"
    echo '# dummy' > "$dirpart/$file"
  done
done
])# _AM_OUTPUT_DEPENDENCY_COMMANDS


# AM_OUTPUT_DEPENDENCY_COMMANDS
# -----------------------------
# This macro should only be invoked once -- use via AC_REQUIRE.
#
# This code is only required when automatic dependency tracking
# is enabled.  FIXME.  This creates each `.P' file that we will
# need in order to bootstrap the dependency handling code.
AC_DEFUN([AM_OUTPUT_DEPENDENCY_COMMANDS],
[AC_CONFIG_COMMANDS([depfiles],
     [test x"$AMDEP_TRUE" != x"" || _AM_OUTPUT_DEPENDENCY_COMMANDS],
     [AMDEP_TRUE="$AMDEP_TRUE" ac_aux_dir="$ac_aux_dir"])
])

# Check to see how 'make' treats includes.	-*- Autoconf -*-

# Copyright (C) 2001, 2002 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 2

# AM_MAKE_INCLUDE()
# -----------------
# Check to see how make treats includes.
AC_DEFUN([AM_MAKE_INCLUDE],
[am_make=${MAKE-make}
cat > confinc << 'END'
doit:
	@echo done
END
# If we don't find an include directive, just comment out the code.
AC_MSG_CHECKING([for style of include used by $am_make])
am__include="#"
am__quote=
_am_result=none
# First try GNU make style include.
echo "include confinc" > confmf
# We grep out `Entering directory' and `Leaving directory'
# messages which can occur if `w' ends up in MAKEFLAGS.
# In particular we don't look at `^make:' because GNU make might
# be invoked under some other name (usually "gmake"), in which
# case it prints its new name instead of `make'.
if test "`$am_make -s -f confmf 2> /dev/null | grep -v 'ing directory'`" = "done"; then
   am__include=include
   am__quote=
   _am_result=GNU
fi
# Now try BSD make style include.
if test "$am__include" = "#"; then
   echo '.include "confinc"' > confmf
   if test "`$am_make -s -f confmf 2> /dev/null`" = "done"; then
      am__include=.include
      am__quote="\""
      _am_result=BSD
   fi
fi
AC_SUBST(am__include)
AC_SUBST(am__quote)
AC_MSG_RESULT($_am_result)
rm -f confinc confmf
])

# AM_CONDITIONAL                                              -*- Autoconf -*-

# Copyright 1997, 2000, 2001 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 5

AC_PREREQ(2.52)

# AM_CONDITIONAL(NAME, SHELL-CONDITION)
# -------------------------------------
# Define a conditional.
AC_DEFUN([AM_CONDITIONAL],
[ifelse([$1], [TRUE],  [AC_FATAL([$0: invalid condition: $1])],
        [$1], [FALSE], [AC_FATAL([$0: invalid condition: $1])])dnl
AC_SUBST([$1_TRUE])
AC_SUBST([$1_FALSE])
if $2; then
  $1_TRUE=
  $1_FALSE='#'
else
  $1_TRUE='#'
  $1_FALSE=
fi
AC_CONFIG_COMMANDS_PRE(
[if test -z "${$1_TRUE}" && test -z "${$1_FALSE}"; then
  AC_MSG_ERROR([conditional "$1" was never defined.
Usually this means the macro was only invoked conditionally.])
fi])])

#serial 5

dnl From Jim Meyering.
dnl Find a new-enough version of Perl.
dnl

AC_DEFUN([jm_PERL],
[
  dnl FIXME: don't hard-code 5.003
  dnl FIXME: should we cache the result?
  AC_MSG_CHECKING([for perl5.003 or newer])
  if test "${PERL+set}" = set; then
    # `PERL' is set in the user's environment.
    candidate_perl_names="$PERL"
    perl_specified=yes
  else
    candidate_perl_names='perl perl5'
    perl_specified=no
  fi

  found=no
  AC_SUBST(PERL)
  PERL="$am_missing_run perl"
  for perl in $candidate_perl_names; do
    # Run test in a subshell; some versions of sh will print an error if
    # an executable is not found, even if stderr is redirected.
    if ( $perl -e 'require 5.003; use File::Compare' ) > /dev/null 2>&1; then
      PERL=$perl
      found=yes
      break
    fi
  done

  AC_MSG_RESULT($found)
  test $found = no && AC_MSG_WARN([
WARNING: You don't seem to have perl5.003 or newer installed, or you lack
         a usable version of the Perl File::Compare module.  As a result,
         you may be unable to run a few tests or to regenerate certain
         files if you modify the sources from which they are derived.
] )
])

#serial 61   -*- autoconf -*-

m4_undefine([AC_LANG_SOURCE(C)])
dnl The following is identical to the definition in c.m4
dnl from the autoconf cvs repository on 2003-03-07.
dnl FIXME: remove this code once we upgrade to autoconf-2.58.

# We can't use '#line $LINENO "configure"' here, since
# Sun c89 (Sun WorkShop 6 update 2 C 5.3 Patch 111679-08 2002/05/09)
# rejects $LINENO greater than 32767, and some configure scripts
# are longer than 32767 lines.
m4_define([AC_LANG_SOURCE(C)],
[/* confdefs.h.  */
_ACEOF
cat confdefs.h >>conftest.$ac_ext
cat >>conftest.$ac_ext <<_ACEOF
/* end confdefs.h.  */
$1])


dnl Misc type-related macros for fileutils, sh-utils, textutils.

AC_DEFUN([jm_MACROS],
[
  AC_PREREQ(2.56)

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
  AC_REQUIRE([AC_FUNC_FTW])
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

  # This is for od and stat, and any other program that
  # uses the PRI.MAX macros from inttypes.h.
  AC_REQUIRE([gt_INTTYPES_PRI])

  AC_REQUIRE([jm_FUNC_GETGROUPS])

  AC_REQUIRE([AC_FUNC_FSEEKO])
  AC_REQUIRE([AC_FUNC_VPRINTF])
  AC_REQUIRE([AC_FUNC_ALLOCA])

  AC_CONFIG_LIBOBJ_DIR([lib])
  AC_FUNC_GETLOADAVG

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

  # raise is used by at least sort and ls.
  AC_REPLACE_FUNCS(raise)

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
    sethostname \
    strchr \
    strerror \
    strrchr \
    sysctl \
    sysinfo \
    wcrtomb \
    tzset \
  )

  # for test.c
  AC_CHECK_FUNCS(setreuid setregid)

  AM_FUNC_GETLINE
  if test $am_cv_func_working_getline != yes; then
    AC_CHECK_FUNCS(getdelim)
  fi
  AC_FUNC_OBSTACK

  AC_FUNC_STRTOD
  AC_REQUIRE([UTILS_SYS_OPEN_MAX])
  AC_REQUIRE([GL_FUNC_GETCWD_PATH_MAX])

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
    DF_PROG='df$(EXEEXT)'
    AC_LIBOBJ(fsusage)
    AC_LIBOBJ(mountlist)
  fi
  AC_REQUIRE([jm_AC_DOS])
  AC_REQUIRE([AC_FUNC_CANONICALIZE_FILE_NAME])

  # If any of these functions don't exist (e.g. DJGPP 2.03),
  # use the corresponding stub.
  AC_CHECK_FUNC([fchdir], , [AC_LIBOBJ(fchdir-stub)])
  AC_CHECK_FUNC([fchown], , [AC_LIBOBJ(fchown-stub)])
  AC_CHECK_FUNC([lstat], , [AC_LIBOBJ(lstat-stub)])
  AC_CHECK_FUNC([readlink], , [AC_LIBOBJ(readlink-stub)])

])

# These tests must be run before any use of AC_CHECK_TYPE,
# because that macro compiles code that tests e.g., HAVE_UNISTD_H.
# See the definition of ac_includes_default in `configure'.
AC_DEFUN([jm_CHECK_ALL_HEADERS],
[
  AC_CHECK_HEADERS( \
    errno.h  \
    fcntl.h \
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

# isc-posix.m4 serial 2 (gettext-0.11.2)
dnl Copyright (C) 1995-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

# This file is not needed with autoconf-2.53 and newer.  Remove it in 2005.

# This test replaces the one in autoconf.
# Currently this macro should have the same name as the autoconf macro
# because gettext's gettext.m4 (distributed in the automake package)
# still uses it.  Otherwise, the use in gettext.m4 makes autoheader
# give these diagnostics:
#   configure.in:556: AC_TRY_COMPILE was called before AC_ISC_POSIX
#   configure.in:556: AC_TRY_RUN was called before AC_ISC_POSIX

undefine([AC_ISC_POSIX])

AC_DEFUN([AC_ISC_POSIX],
  [
    dnl This test replaces the obsolescent AC_ISC_POSIX kludge.
    AC_CHECK_LIB(cposix, strerror, [LIBS="$LIBS -lcposix"])
  ]
)

#serial 15

dnl Initially derived from code in GNU grep.
dnl Mostly written by Jim Meyering.

dnl Usage: jm_INCLUDED_REGEX([lib/regex.c])
dnl
AC_DEFUN([jm_INCLUDED_REGEX],
  [
    dnl Even packages that don't use regex.c can use this macro.
    dnl Of course, for them it doesn't do anything.

    # Assume we'll default to using the included regex.c.
    ac_use_included_regex=yes

    # However, if the system regex support is good enough that it passes the
    # the following run test, then default to *not* using the included regex.c.
    # If cross compiling, assume the test would fail and use the included
    # regex.c.  The first failing regular expression is from `Spencer ere
    # test #75' in grep-2.3.
    AC_CACHE_CHECK([for working re_compile_pattern],
		   jm_cv_func_working_re_compile_pattern,
      AC_TRY_RUN(
[#include <stdio.h>
#include <string.h>
#include <regex.h>
	  int
	  main ()
	  {
	    static struct re_pattern_buffer regex;
	    const char *s;
	    struct re_registers regs;
	    re_set_syntax (RE_SYNTAX_POSIX_EGREP);
	    memset (&regex, 0, sizeof (regex));
	    [s = re_compile_pattern ("a[[:@:>@:]]b\n", 9, &regex);]
	    /* This should fail with _Invalid character class name_ error.  */
	    if (!s)
	      exit (1);

	    /* This should succeed, but doesn't for e.g. glibc-2.1.3.  */
	    memset (&regex, 0, sizeof (regex));
	    s = re_compile_pattern ("{1", 2, &regex);

	    if (s)
	      exit (1);

	    /* The following example is derived from a problem report
               against gawk from Jorge Stolfi <stolfi@ic.unicamp.br>.  */
	    memset (&regex, 0, sizeof (regex));
	    s = re_compile_pattern ("[[anù]]*n", 7, &regex);
	    if (s)
	      exit (1);

	    /* This should match, but doesn't for e.g. glibc-2.2.1.  */
	    if (re_match (&regex, "an", 2, 0, &regs) != 2)
	      exit (1);

	    memset (&regex, 0, sizeof (regex));
	    s = re_compile_pattern ("x", 1, &regex);
	    if (s)
	      exit (1);

	    /* The version of regex.c in e.g. GNU libc-2.2.93 didn't
	       work with a negative RANGE argument.  */
	    if (re_search (&regex, "wxy", 3, 2, -2, &regs) != 1)
	      exit (1);

	    exit (0);
	  }
	],
	       jm_cv_func_working_re_compile_pattern=yes,
	       jm_cv_func_working_re_compile_pattern=no,
	       dnl When crosscompiling, assume it's broken.
	       jm_cv_func_working_re_compile_pattern=no))
    if test $jm_cv_func_working_re_compile_pattern = yes; then
      ac_use_included_regex=no
    fi

    test -n "$1" || AC_MSG_ERROR([missing argument])
    m4_syscmd([test -f $1])
    ifelse(m4_sysval, 0,
      [
	AC_ARG_WITH(included-regex,
	[  --without-included-regex don't compile regex; this is the default on
                          systems with version 2 of the GNU C library
                          (use with caution on other system)],
		    jm_with_regex=$withval,
		    jm_with_regex=$ac_use_included_regex)
	if test "$jm_with_regex" = yes; then
	  AC_LIBOBJ(regex)
	  jm_PREREQ_REGEX
	fi
      ],
    )
  ]
)

# Prerequisites of lib/regex.c.
AC_DEFUN([jm_PREREQ_REGEX],
[
  dnl FIXME: Maybe provide a btowc replacement someday: solaris-2.5.1 lacks it.
  dnl FIXME: Check for wctype and iswctype, and and add -lw if necessary
  dnl to get them.

  dnl Persuade glibc <string.h> to declare mempcpy().
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_REQUIRE([ACX_C_RESTRICT])
  AC_REQUIRE([AC_FUNC_ALLOCA])
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS_ONCE(limits.h string.h wchar.h wctype.h)
  AC_CHECK_FUNCS_ONCE(isascii mempcpy)
  AC_CHECK_FUNCS(btowc)
])

#serial 1001
dnl based on acx_restrict.m4, from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/acx_restrict.html

# Determine whether the C/C++ compiler supports the "restrict" keyword
# introduced in ANSI C99, or an equivalent.  Do nothing if the compiler
# accepts it.  Otherwise, if the compiler supports an equivalent (like
# gcc's __restrict__) define "restrict" to be that.  Otherwise, define
# "restrict" to be empty.

AC_DEFUN([ACX_C_RESTRICT],
[AC_CACHE_CHECK([for C/C++ restrict keyword], acx_cv_c_restrict,
  [acx_cv_c_restrict=no
   # Try the official restrict keyword, then gcc's __restrict__.
   for acx_kw in restrict __restrict__; do
     AC_COMPILE_IFELSE([AC_LANG_SOURCE(
      [float * $acx_kw x;])],
      [acx_cv_c_restrict=$acx_kw; break])
   done
  ])
 case $acx_cv_c_restrict in
   restrict) ;;
   no) AC_DEFINE(restrict,,
	[Define to equivalent of C99 restrict keyword, or to nothing if this
	is not supported.  Do not define if restrict is supported directly.]) ;;
   *)  AC_DEFINE_UNQUOTED(restrict, $acx_cv_c_restrict) ;;
 esac
])

# onceonly.m4 serial 2
dnl Copyright (C) 2002, 2003 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl This file defines some "once only" variants of standard autoconf macros.
dnl   AC_CHECK_HEADERS_ONCE         like  AC_CHECK_HEADERS
dnl   AC_CHECK_FUNCS_ONCE           like  AC_CHECK_FUNCS
dnl   AC_CHECK_DECLS_ONCE           like  AC_CHECK_DECLS
dnl   AC_REQUIRE([AC_HEADER_STDC])  like  AC_HEADER_STDC
dnl The advantage is that the check for each of the headers/functions/decls
dnl will be put only once into the 'configure' file. It keeps the size of
dnl the 'configure' file down, and avoids redundant output when 'configure'
dnl is run.
dnl The drawback is that the checks cannot be conditionalized. If you write
dnl   if some_condition; then gl_CHECK_HEADERS(stdlib.h); fi
dnl inside an AC_DEFUNed function, the gl_CHECK_HEADERS macro call expands to
dnl empty, and the check will be inserted before the body of the AC_DEFUNed
dnl function.

dnl Taken from Autoconf 2.50; can be removed once we assume 2.50 or later.
define([m4_quote], [[$*]])

# AC_CHECK_HEADERS_ONCE(HEADER1 HEADER2 ...) is a once-only variant of
# AC_CHECK_HEADERS(HEADER1 HEADER2 ...).
AC_DEFUN([AC_CHECK_HEADERS_ONCE], [
  :
  AC_FOREACH([gl_HEADER_NAME], [$1], [
    AC_DEFUN([gl_CHECK_HEADER_]m4_quote(translit(defn([gl_HEADER_NAME]),
						 [-./], [___])), [
      AC_CHECK_HEADERS(gl_HEADER_NAME)
    ])
    AC_REQUIRE([gl_CHECK_HEADER_]m4_quote(translit(gl_HEADER_NAME,
						   [-./], [___])))
  ])
])

# AC_CHECK_FUNCS_ONCE(FUNC1 FUNC2 ...) is a once-only variant of
# AC_CHECK_FUNCS(FUNC1 FUNC2 ...).
AC_DEFUN([AC_CHECK_FUNCS_ONCE], [
  :
  AC_FOREACH([gl_FUNC_NAME], [$1], [
    AC_DEFUN([gl_CHECK_FUNC_]defn([gl_FUNC_NAME]), [
      AC_CHECK_FUNCS(defn([gl_FUNC_NAME]))
    ])
    AC_REQUIRE([gl_CHECK_FUNC_]defn([gl_FUNC_NAME]))
  ])
])

# AC_CHECK_DECLS_ONCE(DECL1 DECL2 ...) is a once-only variant of
# AC_CHECK_DECLS(DECL1, DECL2, ...).
AC_DEFUN([AC_CHECK_DECLS_ONCE], [
  :
  AC_FOREACH([gl_DECL_NAME], [$1], [
    AC_DEFUN([gl_CHECK_DECL_]defn([gl_DECL_NAME]), [
      AC_CHECK_DECLS(defn([gl_DECL_NAME]))
    ])
    AC_REQUIRE([gl_CHECK_DECL_]defn([gl_DECL_NAME]))
  ])
])

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

#serial 1

# On some systems, mkdir ("foo/", 0700) fails because of the trailing slash.
# On such systems, arrange to use a wrapper function that removes any
# trailing slashes.
AC_DEFUN([UTILS_FUNC_MKDIR_TRAILING_SLASH],
[dnl
  AC_CACHE_CHECK([whether mkdir fails due to a trailing slash],
    utils_cv_func_mkdir_trailing_slash_bug,
    [
      # Arrange for deletion of the temporary directory this test might create.
      ac_clean_files="$ac_clean_files confdir-slash"
      AC_TRY_RUN([
#       include <sys/types.h>
#       include <sys/stat.h>
#       include <stdlib.h>
	int main ()
	{
	  rmdir ("confdir-slash");
	  exit (mkdir ("confdir-slash/", 0700));
	}
	],
      utils_cv_func_mkdir_trailing_slash_bug=no,
      utils_cv_func_mkdir_trailing_slash_bug=yes,
      utils_cv_func_mkdir_trailing_slash_bug=yes
      )
    ]
  )

  if test $utils_cv_func_mkdir_trailing_slash_bug = yes; then
    AC_LIBOBJ(mkdir)
    AC_DEFINE(mkdir, rpl_mkdir,
      [Define to rpl_mkdir if the replacement function should be used.])
  fi
])

#serial 2

AC_DEFUN([jm_BISON],
[
  # getdate.y works with bison only.
  : ${YACC='bison -y'}
  AC_SUBST(YACC)
])

#serial 3
dnl based on code from Eleftherios Gkioulekas

AC_DEFUN([jm_ASSERT],
[
  AC_MSG_CHECKING(whether to enable assertions)
  AC_ARG_ENABLE(assert,
	[  --disable-assert        turn off assertions],
	[ AC_MSG_RESULT(no)
	  AC_DEFINE(NDEBUG,1,[Define to 1 if assertions should be disabled.]) ],
	[ AC_MSG_RESULT(yes) ]
               )
])

#serial 4

dnl From Jim Meyering

dnl Define HAVE_STRUCT_UTIMBUF if `struct utimbuf' is declared --
dnl usually in <utime.h>.
dnl Some systems have utime.h but don't declare the struct anywhere.

AC_DEFUN([jm_CHECK_TYPE_STRUCT_UTIMBUF],
[
  AC_CHECK_HEADERS(utime.h)
  AC_REQUIRE([AC_HEADER_TIME])
  AC_CACHE_CHECK([for struct utimbuf], fu_cv_sys_struct_utimbuf,
    [AC_TRY_COMPILE(
      [
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_UTIME_H
# include <utime.h>
#endif
      ],
      [static struct utimbuf x; x.actime = x.modtime;],
      fu_cv_sys_struct_utimbuf=yes,
      fu_cv_sys_struct_utimbuf=no)
    ])

  if test $fu_cv_sys_struct_utimbuf = yes; then
    AC_DEFINE(HAVE_STRUCT_UTIMBUF, 1,
[Define if struct utimbuf is declared -- usually in <utime.h>.
   Some systems have utime.h but don't declare the struct anywhere. ])
  fi
])

#serial 5

dnl From Jim Meyering.
dnl
dnl Check whether struct dirent has a member named d_type.
dnl

AC_DEFUN([jm_CHECK_TYPE_STRUCT_DIRENT_D_TYPE],
  [AC_REQUIRE([AC_HEADER_DIRENT])dnl
   AC_CACHE_CHECK([for d_type member in directory struct],
		  jm_cv_struct_dirent_d_type,
     [AC_TRY_LINK(dnl
       [
#include <sys/types.h>
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#else /* not HAVE_DIRENT_H */
# define dirent direct
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H */
       ],
       [struct dirent dp; dp.d_type = 0;],

       jm_cv_struct_dirent_d_type=yes,
       jm_cv_struct_dirent_d_type=no)
     ]
   )
   if test $jm_cv_struct_dirent_d_type = yes; then
     AC_DEFINE(HAVE_STRUCT_DIRENT_D_TYPE, 1,
  [Define if there is a member named d_type in the struct describing
   directory headers.])
   fi
  ]
)

#serial 4

dnl From Jim Meyering.
dnl
dnl Check whether struct dirent has a member named d_ino.
dnl

AC_DEFUN([jm_CHECK_TYPE_STRUCT_DIRENT_D_INO],
  [AC_REQUIRE([AC_HEADER_DIRENT])dnl
   AC_CACHE_CHECK([for d_ino member in directory struct],
		  jm_cv_struct_dirent_d_ino,
     [AC_TRY_LINK(dnl
       [
#include <sys/types.h>
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#else /* not HAVE_DIRENT_H */
# define dirent direct
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H */
       ],
       [struct dirent dp; dp.d_ino = 0;],

       jm_cv_struct_dirent_d_ino=yes,
       jm_cv_struct_dirent_d_ino=no)
     ]
   )
   if test $jm_cv_struct_dirent_d_ino = yes; then
     AC_DEFINE(D_INO_IN_DIRENT, 1,
  [Define if there is a member named d_ino in the struct describing
   directory headers.])
   fi
  ]
)

#serial 19

dnl This is just a wrapper function to encapsulate this kludge.
dnl Putting it in a separate file like this helps share it between
dnl different packages.
AC_DEFUN([jm_CHECK_DECLS],
[
  AC_REQUIRE([_jm_DECL_HEADERS])
  AC_REQUIRE([AC_HEADER_TIME])
  headers='
#include <stdio.h>
#if HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#else
# if HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <sys/types.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_UTMP_H
# include <utmp.h>
#endif

#if HAVE_GRP_H
# include <grp.h>
#endif

#if HAVE_PWD_H
# include <pwd.h>
#endif
'

  AC_CHECK_DECLS([
    euidaccess,
    free,
    getenv,
    geteuid,
    getgrgid,
    getlogin,
    getpwuid,
    getuid,
    getutent,
    lseek,
    malloc,
    memchr,
    memrchr,
    nanosleep,
    realloc,
    stpcpy,
    strndup,
    strnlen,
    strstr,
    strtoul,
    strtoull,
    ttyname], , , $headers)
])

dnl FIXME: when autoconf has support for it.
dnl This is a little helper so we can require these header checks.
AC_DEFUN([_jm_DECL_HEADERS],
[
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS(grp.h memory.h pwd.h string.h strings.h stdlib.h \
                   unistd.h sys/time.h utmp.h utmpx.h)
])

#serial 31

dnl We use jm_ for non Autoconf macros.
m4_pattern_forbid([^jm_[ABCDEFGHIJKLMNOPQRSTUVXYZ]])dnl
m4_pattern_forbid([^gl_[ABCDEFGHIJKLMNOPQRSTUVXYZ]])dnl

# These are the prerequisite macros for files in the lib/
# directory of the coreutils package.

AC_DEFUN([jm_PREREQ],
[
  AC_REQUIRE([jm_PREREQ_ADDEXT])

  # We don't yet use c-stack.c.
  # AC_REQUIRE([jm_PREREQ_C_STACK])

  AC_REQUIRE([jm_PREREQ_CANON_HOST])
  AC_REQUIRE([jm_PREREQ_DIRNAME])
  AC_REQUIRE([jm_PREREQ_ERROR])
  AC_REQUIRE([jm_PREREQ_EXCLUDE])
  AC_REQUIRE([jm_PREREQ_GETPAGESIZE])
  AC_REQUIRE([jm_PREREQ_HARD_LOCALE])
  AC_REQUIRE([jm_PREREQ_HASH])
  AC_REQUIRE([jm_PREREQ_HUMAN])
  AC_REQUIRE([jm_PREREQ_MBSWIDTH])
  AC_REQUIRE([jm_PREREQ_MEMCHR])
  AC_REQUIRE([jm_PREREQ_PHYSMEM])
  AC_REQUIRE([jm_PREREQ_POSIXVER])
  AC_REQUIRE([jm_PREREQ_QUOTEARG])
  AC_REQUIRE([jm_PREREQ_READUTMP])
  AC_REQUIRE([jm_PREREQ_STAT])
  AC_REQUIRE([jm_PREREQ_STRNLEN])
  AC_REQUIRE([jm_PREREQ_TEMPNAME]) # called by mkstemp
  AC_REQUIRE([jm_PREREQ_XGETCWD])
  AC_REQUIRE([jm_PREREQ_XREADLINK])
])

AC_DEFUN([jm_PREREQ_ADDEXT],
[
  dnl For addext.c.
  AC_REQUIRE([AC_SYS_LONG_FILE_NAMES])
  AC_CHECK_FUNCS(pathconf)
  AC_CHECK_HEADERS(limits.h string.h unistd.h)
])

AC_DEFUN([jm_PREREQ_CANON_HOST],
[
  dnl Add any libraries as early as possible.
  dnl In particular, inet_ntoa needs -lnsl at least on Solaris5.5.1,
  dnl so we have to add -lnsl to LIBS before checking for that function.
  AC_SEARCH_LIBS(gethostbyname, [inet nsl])

  dnl These come from -lnsl on Solaris5.5.1.
  AC_CHECK_FUNCS(gethostbyname gethostbyaddr inet_ntoa)

  AC_CHECK_HEADERS(unistd.h string.h netdb.h sys/socket.h \
                   netinet/in.h arpa/inet.h)
])

AC_DEFUN([jm_PREREQ_DIRNAME],
[
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS(string.h)
])

AC_DEFUN([jm_PREREQ_EXCLUDE],
[
  AC_REQUIRE([AC_FUNC_FNMATCH_GNU])
  AC_REQUIRE([AC_HEADER_STDBOOL])
])

AC_DEFUN([jm_PREREQ_GETPAGESIZE],
[
  AC_CHECK_FUNCS(getpagesize)
  AC_CHECK_HEADERS(OS.h unistd.h)
])

AC_DEFUN([jm_PREREQ_HARD_LOCALE],
[
  AC_CHECK_HEADERS(locale.h stdlib.h string.h)
  AC_CHECK_FUNCS(setlocale)
  AC_REQUIRE([AM_C_PROTOTYPES])
])

AC_DEFUN([jm_PREREQ_HASH],
[
  AC_CHECK_HEADERS(stdlib.h)
  AC_REQUIRE([AC_HEADER_STDBOOL])
  AC_REQUIRE([jm_CHECK_DECLS])
])

# If you use human.c, you need the following files:
# inttypes.m4 longlong.m4
AC_DEFUN([jm_PREREQ_HUMAN],
[
  AC_CHECK_HEADERS(locale.h)
  AC_CHECK_DECLS([getenv])
  AC_CHECK_FUNCS(localeconv)
  AC_REQUIRE([AC_HEADER_STDBOOL])
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
])

AC_DEFUN([jm_PREREQ_MEMCHR],
[
  AC_CHECK_HEADERS(limits.h stdlib.h bp-sym.h)
])

# Check for the external symbol, _system_configuration,
# a struct with member `physmem'.
AC_DEFUN([gl_SYS__SYSTEM_CONFIGURATION],
  [AC_CACHE_CHECK(for external symbol _system_configuration,
		  gl_cv_var__system_configuration,
    [AC_LINK_IFELSE([AC_LANG_PROGRAM(
		      [[#include <sys/systemcfg.h>
		      ]],
		      [double x = _system_configuration.physmem;])],
      [gl_cv_var__system_configuration=yes],
      [gl_cv_var__system_configuration=no])])

    if test $gl_cv_var__system_configuration = yes; then
      AC_DEFINE(HAVE__SYSTEM_CONFIGURATION, 1,
		[Define to 1 if you have the external variable,
		_system_configuration with a member named physmem.])
    fi
  ]
)

AC_DEFUN([jm_PREREQ_PHYSMEM],
[
  AC_CHECK_HEADERS([unistd.h sys/pstat.h sys/sysmp.h sys/sysinfo.h \
    machine/hal_sysinfo.h sys/table.h sys/param.h sys/sysctl.h \
    sys/systemcfg.h])
  AC_CHECK_FUNCS(pstat_getstatic pstat_getdynamic sysmp getsysinfo sysctl table)

  AC_REQUIRE([gl_SYS__SYSTEM_CONFIGURATION])
])

AC_DEFUN([jm_PREREQ_POSIXVER],
[
  AC_CHECK_HEADERS(unistd.h)
  AC_CHECK_DECLS([getenv])
])

AC_DEFUN([jm_PREREQ_QUOTEARG],
[
  AC_CHECK_FUNCS(isascii iswprint)
  AC_REQUIRE([jm_FUNC_MBRTOWC])
  AC_REQUIRE([jm_FUNC_MEMCMP])
  AC_CHECK_HEADERS(limits.h stddef.h stdlib.h string.h wchar.h wctype.h)
  AC_REQUIRE([AC_HEADER_STDC])
  AC_REQUIRE([AC_C_BACKSLASH_A])
  AC_REQUIRE([AC_TYPE_MBSTATE_T])
  AC_REQUIRE([AM_C_PROTOTYPES])
])

AC_DEFUN([jm_PREREQ_READUTMP],
[
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS(string.h utmp.h utmpx.h sys/param.h)
  AC_CHECK_FUNCS(utmpname)
  AC_CHECK_FUNCS(utmpxname)
  AC_REQUIRE([AM_C_PROTOTYPES])

  if test $ac_cv_header_utmp_h = yes || test $ac_cv_header_utmpx_h = yes; then
    utmp_includes="\
$ac_includes_default
#ifdef HAVE_UTMPX_H
# include <utmpx.h>
#endif
#ifdef HAVE_UTMP_H
# include <utmp.h>
#endif
"
    AC_CHECK_MEMBERS([struct utmpx.ut_user],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_user],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_name],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_name],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_type],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_type],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_pid],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_pid],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_id],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_id],,,[$utmp_includes])

    AC_CHECK_MEMBERS([struct utmpx.ut_exit.ut_exit],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit.ut_exit],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_exit.e_exit],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit.e_exit],,,[$utmp_includes])

    AC_CHECK_MEMBERS([struct utmpx.ut_exit.ut_termination],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit.ut_termination],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_exit.e_termination],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit.e_termination],,,[$utmp_includes])
    AC_LIBOBJ(readutmp)
  fi
])

AC_DEFUN([jm_PREREQ_STAT],
[
  AC_CHECK_HEADERS(sys/sysmacros.h sys/statvfs.h sys/vfs.h inttypes.h)
  AC_CHECK_HEADERS(sys/param.h sys/mount.h)
  AC_CHECK_FUNCS(statvfs)
  AC_REQUIRE([jm_AC_TYPE_LONG_LONG])

  statxfs_includes="\
$ac_includes_default
#if HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#endif
#if HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif
#if ( ! HAVE_SYS_STATVFS_H && ! HAVE_SYS_VFS_H && HAVE_SYS_MOUNT_H && HAVE_SYS_PARAM_H )
/* NetBSD 1.5.2 needs these, for the declaration of struct statfs. */
# include <sys/param.h>
# include <sys/mount.h>
#endif
"
  AC_CHECK_MEMBERS([struct statfs.f_basetype],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_basetype],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_fstypename],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_type],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_type],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_fsid.__val],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_fsid.__val],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_namemax],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_namemax],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_namelen],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_namelen],,,[$statxfs_includes])
])

AC_DEFUN([jm_PREREQ_STRNLEN],
[
  AC_REQUIRE([AC_FUNC_STRNLEN])
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS(memory.h)
  AC_CHECK_DECLS([memchr])

  # This is necessary because automake-1.6.1 doesn't understand
  # that the above use of AC_FUNC_STRNLEN means we may have to use
  # lib/strnlen.c.
  test $ac_cv_func_strnlen_working = yes \
    && AC_LIBOBJ(strnlen)
])

AC_DEFUN([jm_PREREQ_TEMPNAME],
[
  AC_REQUIRE([AC_HEADER_STDC])
  AC_REQUIRE([AC_HEADER_STAT])
  AC_CHECK_HEADERS(fcntl.h sys/time.h stdint.h unistd.h)
  AC_CHECK_FUNCS(__secure_getenv gettimeofday)
  AC_CHECK_DECLS([getenv])
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
])

AC_DEFUN([jm_PREREQ_XGETCWD],
[
  AC_REQUIRE([AC_C_PROTOTYPES])
  AC_CHECK_HEADERS(limits.h stdlib.h sys/param.h unistd.h)
  AC_CHECK_FUNCS(getcwd)
  AC_REQUIRE([AC_FUNC_GETCWD_NULL])
])

AC_DEFUN([jm_PREREQ_XREADLINK],
[
  AC_REQUIRE([AC_C_PROTOTYPES])
  AC_CHECK_HEADERS(limits.h stdlib.h sys/types.h unistd.h)
])

#serial 5

dnl FIXME: put these prerequisite-only *.m4 files in a separate
dnl directory -- otherwise, they'll conflict with existing files.

dnl These are the prerequisite macros for GNU's error.c file.
AC_DEFUN([jm_PREREQ_ERROR],
[
  AC_CHECK_FUNCS(strerror vprintf doprnt)
  AC_CHECK_DECLS([strerror])
  AC_CHECK_HEADERS([libintl.h])
  AC_FUNC_STRERROR_R
  AC_HEADER_STDC
])

#serial 7

dnl autoconf tests required for use of mbswidth.c
dnl From Bruno Haible.

AC_DEFUN([jm_PREREQ_MBSWIDTH],
[
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS(limits.h stdlib.h string.h wchar.h wctype.h)
  AC_CHECK_FUNCS(isascii iswcntrl iswprint mbsinit wcwidth)
  jm_FUNC_MBRTOWC

  AC_CACHE_CHECK([whether wcwidth is declared], ac_cv_have_decl_wcwidth,
    [AC_TRY_COMPILE([
/* AIX 3.2.5 declares wcwidth in <string.h>. */
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_WCHAR_H
# include <wchar.h>
#endif
], [
#ifndef wcwidth
  char *p = (char *) wcwidth;
#endif
], ac_cv_have_decl_wcwidth=yes, ac_cv_have_decl_wcwidth=no)])
  if test $ac_cv_have_decl_wcwidth = yes; then
    ac_val=1
  else
    ac_val=0
  fi
  AC_DEFINE_UNQUOTED(HAVE_DECL_WCWIDTH, $ac_val,
    [Define to 1 if you have the declaration of wcwidth(), and to 0 otherwise.])

  AC_TYPE_MBSTATE_T
])

# mbrtowc.m4 serial 4 (fileutils-4.1.3)
dnl Copyright (C) 2001-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl From Paul Eggert

AC_DEFUN([jm_FUNC_MBRTOWC],
[
  AC_CACHE_CHECK([whether mbrtowc and mbstate_t are properly declared],
    jm_cv_func_mbrtowc,
    [AC_TRY_LINK(
       [#include <wchar.h>],
       [mbstate_t state; return ! (sizeof state && mbrtowc);],
       jm_cv_func_mbrtowc=yes,
       jm_cv_func_mbrtowc=no)])
  if test $jm_cv_func_mbrtowc = yes; then
    AC_DEFINE(HAVE_MBRTOWC, 1,
      [Define to 1 if mbrtowc and mbstate_t are properly declared.])
  fi
])

#serial 5

dnl A replacement for autoconf's macro by the same name.  This version
dnl uses `ac_lib' rather than `i' for the loop variable, but more importantly
dnl moves the ACTION-IF-FOUND ([$]3) into the inner `if'-block so that it is
dnl run only if one of the listed libraries ends up being used (and not in
dnl the `none required' case.
dnl I hope it's only temporary while we wait for that version to be fixed.
undefine([AC_SEARCH_LIBS])

# AC_SEARCH_LIBS(FUNCTION, SEARCH-LIBS,
#                [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#                [OTHER-LIBRARIES])
# --------------------------------------------------------
# Search for a library defining FUNC, if it's not already available.
AC_DEFUN([AC_SEARCH_LIBS],
[
  AC_CACHE_CHECK([for library containing $1], [ac_cv_search_$1],
  [
    ac_func_search_save_LIBS=$LIBS
    ac_cv_search_$1=no
    AC_TRY_LINK_FUNC([$1], [ac_cv_search_$1='none required'])
    if test "$ac_cv_search_$1" = no; then
      for ac_lib in $2; do
	LIBS="-l$ac_lib $5 $ac_func_search_save_LIBS"
	AC_TRY_LINK_FUNC([$1], [ac_cv_search_$1="-l$ac_lib"; break])
      done
    fi
    LIBS=$ac_func_search_save_LIBS
  ])

  if test "$ac_cv_search_$1" = no; then :
    $4
  else
    if test "$ac_cv_search_$1" = 'none required'; then :
      $4
    else
      LIBS="$ac_cv_search_$1 $LIBS"
      $3
    fi
  fi
])

# Check for stdbool.h that conforms to C99.

# Copyright (C) 2002 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

AC_DEFUN([AC_HEADER_STDBOOL],
  [AC_CACHE_CHECK([for stdbool.h that conforms to C99],
     [ac_cv_header_stdbool_h],
     [AC_TRY_COMPILE(
	[
	  #include <stdbool.h>
	  #ifndef bool
	   "error: bool is not defined"
	  #endif
	  #ifndef false
	   "error: false is not defined"
	  #endif
	  #if false
	   "error: false is not 0"
	  #endif
	  #ifndef true
	   "error: false is not defined"
	  #endif
	  #if true != 1
	   "error: true is not 1"
	  #endif
	  #ifndef __bool_true_false_are_defined
	   "error: __bool_true_false_are_defined is not defined"
	  #endif

	  struct s { _Bool s: 1; _Bool t; } s;

	  char a[true == 1 ? 1 : -1];
	  char b[false == 0 ? 1 : -1];
	  char c[__bool_true_false_are_defined == 1 ? 1 : -1];
	  char d[(bool) -0.5 == true ? 1 : -1];
	  bool e = &s;
	  char f[(_Bool) -0.0 == false ? 1 : -1];
	  char g[true];
	  char h[sizeof (_Bool)];
	  char i[sizeof s.t];
	],
	[ return !a + !b + !c + !d + !e + !f + !g + !h + !i; ],
	[ac_cv_header_stdbool_h=yes],
	[ac_cv_header_stdbool_h=no])])
   if test $ac_cv_header_stdbool_h = yes; then
     AC_DEFINE(HAVE_STDBOOL_H, 1, [Define to 1 if stdbool.h conforms to C99.])
   fi])


# Copyright 1996, 1997, 1998, 2000, 2001, 2002  Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 2

AC_DEFUN([AM_C_PROTOTYPES],
[AC_REQUIRE([AM_PROG_CC_STDC])
AC_REQUIRE([AC_PROG_CPP])
AC_MSG_CHECKING([for function prototypes])
if test "$am_cv_prog_cc_stdc" != no; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(PROTOTYPES,1,[Define if compiler has function prototypes])
  U= ANSI2KNR=
else
  AC_MSG_RESULT(no)
  U=_ ANSI2KNR=./ansi2knr
fi
# Ensure some checks needed by ansi2knr itself.
AC_HEADER_STDC
AC_CHECK_HEADERS(string.h)
AC_SUBST(U)dnl
AC_SUBST(ANSI2KNR)dnl
])

AU_DEFUN([fp_C_PROTOTYPES], [AM_C_PROTOTYPES])


# Copyright 1996, 1997, 1999, 2000, 2001, 2002  Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 2

# @defmac AC_PROG_CC_STDC
# @maindex PROG_CC_STDC
# @ovindex CC
# If the C compiler in not in ANSI C mode by default, try to add an option
# to output variable @code{CC} to make it so.  This macro tries various
# options that select ANSI C on some system or another.  It considers the
# compiler to be in ANSI C mode if it handles function prototypes correctly.
#
# If you use this macro, you should check after calling it whether the C
# compiler has been set to accept ANSI C; if not, the shell variable
# @code{am_cv_prog_cc_stdc} is set to @samp{no}.  If you wrote your source
# code in ANSI C, you can make an un-ANSIfied copy of it by using the
# program @code{ansi2knr}, which comes with Ghostscript.
# @end defmac

AC_DEFUN([AM_PROG_CC_STDC],
[AC_REQUIRE([AC_PROG_CC])
AC_BEFORE([$0], [AC_C_INLINE])
AC_BEFORE([$0], [AC_C_CONST])
dnl Force this before AC_PROG_CPP.  Some cpp's, eg on HPUX, require
dnl a magic option to avoid problems with ANSI preprocessor commands
dnl like #elif.
dnl FIXME: can't do this because then AC_AIX won't work due to a
dnl circular dependency.
dnl AC_BEFORE([$0], [AC_PROG_CPP])
AC_MSG_CHECKING([for ${CC-cc} option to accept ANSI C])
AC_CACHE_VAL(am_cv_prog_cc_stdc,
[am_cv_prog_cc_stdc=no
ac_save_CC="$CC"
# Don't try gcc -ansi; that turns off useful extensions and
# breaks some systems' header files.
# AIX			-qlanglvl=ansi
# Ultrix and OSF/1	-std1
# HP-UX 10.20 and later	-Ae
# HP-UX older versions	-Aa -D_HPUX_SOURCE
# SVR4			-Xc -D__EXTENSIONS__
for ac_arg in "" -qlanglvl=ansi -std1 -Ae "-Aa -D_HPUX_SOURCE" "-Xc -D__EXTENSIONS__"
do
  CC="$ac_save_CC $ac_arg"
  AC_TRY_COMPILE(
[#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
/* Most of the following tests are stolen from RCS 5.7's src/conf.sh.  */
struct buf { int x; };
FILE * (*rcsopen) (struct buf *, struct stat *, int);
static char *e (p, i)
     char **p;
     int i;
{
  return p[i];
}
static char *f (char * (*g) (char **, int), char **p, ...)
{
  char *s;
  va_list v;
  va_start (v,p);
  s = g (p, va_arg (v,int));
  va_end (v);
  return s;
}
int test (int i, double x);
struct s1 {int (*f) (int a);};
struct s2 {int (*f) (double a);};
int pairnames (int, char **, FILE *(*)(struct buf *, struct stat *, int), int, int);
int argc;
char **argv;
], [
return f (e, argv, 0) != argv[0]  ||  f (e, argv, 1) != argv[1];
],
[am_cv_prog_cc_stdc="$ac_arg"; break])
done
CC="$ac_save_CC"
])
if test -z "$am_cv_prog_cc_stdc"; then
  AC_MSG_RESULT([none needed])
else
  AC_MSG_RESULT([$am_cv_prog_cc_stdc])
fi
case "x$am_cv_prog_cc_stdc" in
  x|xno) ;;
  *) CC="$CC $am_cv_prog_cc_stdc" ;;
esac
])

AU_DEFUN([fp_PROG_CC_STDC], [AM_PROG_CC_STDC])

#serial 6

dnl From Paul Eggert.

AC_PREREQ(2.52)

# Define intmax_t to long or long long if <inttypes.h> doesn't define.

AC_DEFUN([jm_AC_TYPE_INTMAX_T],
[
  AC_REQUIRE([jm_AC_TYPE_LONG_LONG])
  AC_CHECK_TYPE(intmax_t, ,
    [test $ac_cv_type_long_long = yes \
       && ac_type='long long' \
       || ac_type='long'
     AC_DEFINE_UNQUOTED(intmax_t, $ac_type,
       [Define to widest signed type if <inttypes.h> doesn't define.])])
])

# Define uintmax_t to unsigned long or unsigned long long
# if <inttypes.h> doesn't define.

AC_DEFUN([jm_AC_TYPE_UINTMAX_T],
[
  AC_REQUIRE([jm_AC_TYPE_UNSIGNED_LONG_LONG])
  AC_CHECK_TYPE(uintmax_t, ,
    [test $ac_cv_type_unsigned_long_long = yes \
       && ac_type='unsigned long long' \
       || ac_type='unsigned long'
     AC_DEFINE_UNQUOTED(uintmax_t, $ac_type,
       [Define to widest unsigned type if <inttypes.h> doesn't define.])])
])

#serial 2

dnl From Paul Eggert.

# Define HAVE_LONG_LONG if 'long long' works.

AC_DEFUN([jm_AC_TYPE_LONG_LONG],
[
  AC_CACHE_CHECK([for long long], ac_cv_type_long_long,
  [AC_TRY_LINK([long long ll = 1; int i = 63;],
    [long long llmax = (long long) -1;
     return ll << i | ll >> i | llmax / ll | llmax % ll;],
    ac_cv_type_long_long=yes,
    ac_cv_type_long_long=no)])
  if test $ac_cv_type_long_long = yes; then
    AC_DEFINE(HAVE_LONG_LONG, 1,
      [Define if you have the long long type.])
  fi
])

# Define HAVE_UNSIGNED_LONG_LONG if 'unsigned long long' works.

AC_DEFUN([jm_AC_TYPE_UNSIGNED_LONG_LONG],
[
  AC_CACHE_CHECK([for unsigned long long], ac_cv_type_unsigned_long_long,
  [AC_TRY_LINK([unsigned long long ull = 1; int i = 63;],
    [unsigned long long ullmax = (unsigned long long) -1;
     return ull << i | ull >> i | ullmax / ull | ullmax % ull;],
    ac_cv_type_unsigned_long_long=yes,
    ac_cv_type_unsigned_long_long=no)])
  if test $ac_cv_type_unsigned_long_long = yes; then
    AC_DEFINE(HAVE_UNSIGNED_LONG_LONG, 1,
      [Define if you have the unsigned long long type.])
  fi
])

#serial 7

AC_DEFUN([jm_FUNC_MEMCMP],
[AC_REQUIRE([AC_FUNC_MEMCMP])dnl
 if test $ac_cv_func_memcmp_working = no; then
   AC_DEFINE(memcmp, rpl_memcmp,
     [Define to rpl_memcmp if the replacement function should be used.])
 fi
])

# getcwd.m4 - check whether getcwd (NULL, 0) allocates memory for result

# Copyright 2001 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

# Written by Paul Eggert.

AC_DEFUN([AC_FUNC_GETCWD_NULL],
  [AC_CHECK_HEADERS(stdlib.h unistd.h)
   AC_CACHE_CHECK([whether getcwd (NULL, 0) allocates memory for result],
     [ac_cv_func_getcwd_null],
     [AC_TRY_RUN(
        [
#	 ifdef HAVE_STDLIB_H
#	  include <stdlib.h>
#	 endif
#	 ifdef HAVE_UNISTD_H
#	  include <unistd.h>
#	 endif
#	 ifndef getcwd
	 char *getcwd ();
#	 endif
	 int
	 main ()
	 {
	   if (chdir ("/") != 0)
	     exit (1);
	   else
	     {
	       char *f = getcwd (NULL, 0);
	       exit (! (f && f[0] == '/' && !f[1]));
	     }
	 }],
	[ac_cv_func_getcwd_null=yes],
	[ac_cv_func_getcwd_null=no],
	[ac_cv_func_getcwd_null=no])])
   if test $ac_cv_func_getcwd_null = yes; then
     AC_DEFINE(HAVE_GETCWD_NULL, 1,
	       [Define if getcwd (NULL, 0) allocates memory for result.])
   fi])

#serial 5

dnl Find out how to get the file descriptor associated with an open DIR*.
dnl From Jim Meyering

AC_DEFUN([UTILS_FUNC_DIRFD],
[
  dnl Work around a bug of AC_EGREP_CPP in autoconf-2.57.
  AC_REQUIRE([AC_PROG_CPP])
  AC_REQUIRE([AC_PROG_EGREP])

  AC_HEADER_DIRENT
  dirfd_headers='
#if HAVE_DIRENT_H
# include <dirent.h>
#else /* not HAVE_DIRENT_H */
# define dirent direct
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# if HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H */
'
  AC_CHECK_FUNCS(dirfd)
  AC_CHECK_DECLS([dirfd], , , $dirfd_headers)

  AC_CACHE_CHECK([whether dirfd is a macro],
    jm_cv_func_dirfd_macro,
    AC_EGREP_CPP([dirent_header_defines_dirfd], [$dirfd_headers
#ifdef dirfd
 dirent_header_defines_dirfd
#endif],
      jm_cv_func_dirfd_macro=yes,
      jm_cv_func_dirfd_macro=no))

  # Use the replacement only if we have no function, macro,
  # or declaration with that name.
  if test $ac_cv_func_dirfd,$ac_cv_have_decl_dirfd,$jm_cv_func_dirfd_macro \
      = no,no,no; then
    AC_REPLACE_FUNCS([dirfd])
    AC_CACHE_CHECK(
	      [how to get the file descriptor associated with an open DIR*],
		   gl_cv_sys_dir_fd_member_name,
      [
        dirfd_save_CFLAGS=$CFLAGS
	for ac_expr in d_fd dd_fd; do

	  CFLAGS="$CFLAGS -DDIR_FD_MEMBER_NAME=$ac_expr"
	  AC_TRY_COMPILE(
	    [$dirfd_headers
	    ],
	    [DIR *dir_p = opendir("."); (void) dir_p->DIR_FD_MEMBER_NAME;],
	    dir_fd_found=yes
	  )
	  CFLAGS=$dirfd_save_CFLAGS
	  test "$dir_fd_found" = yes && break
	done
	test "$dir_fd_found" = yes || ac_expr=no_such_member

	gl_cv_sys_dir_fd_member_name=$ac_expr
      ]
    )
    if test $gl_cv_sys_dir_fd_member_name != no_such_member; then
      AC_DEFINE_UNQUOTED(DIR_FD_MEMBER_NAME,
	$gl_cv_sys_dir_fd_member_name,
	[the name of the file descriptor member of DIR])
    fi
    AH_VERBATIM(DIR_TO_FD,
		[#ifdef DIR_FD_MEMBER_NAME
# define DIR_TO_FD(Dir_p) ((Dir_p)->DIR_FD_MEMBER_NAME)
#else
# define DIR_TO_FD(Dir_p) -1
#endif
]
    )
  fi
])

# acl.m4 - check for access control list (ACL) primitives

# Copyright (C) 2002 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

# Written by Paul Eggert.

AC_DEFUN([AC_FUNC_ACL],
  [AC_CHECK_HEADERS(sys/acl.h)
   AC_CHECK_FUNCS(acl)])

#serial 2
# Use the replacement ftw.c if the one in the C library is inadequate or buggy.
# For now, we always use the code in lib/ because libc doesn't have the FTW_DCH
# or FTW_DCHP that we need.  Arrange to use lib/ftw.h.  And since that
# implementation uses tsearch.c/tdestroy, add tsearch.o to the list of
# objects and arrange to use lib/search.h if necessary.
# From Jim Meyering

AC_DEFUN([AC_FUNC_FTW],
[
  # prerequisites
  AC_REQUIRE([AC_HEADER_STAT])
  AC_REQUIRE([jm_FUNC_LSTAT])
  AC_REQUIRE([AC_HEADER_DIRENT])
  AC_CHECK_HEADERS(sys/param.h)
  AC_CHECK_DECLS([stpcpy])

  # In the event that we have to use the replacement ftw.c,
  # see if we'll also need the replacement tsearch.c.
  AC_CHECK_FUNC([tdestroy], , [need_tdestroy=1])

  AC_CACHE_CHECK([for ftw/FTW_CHDIR that informs callback of failed chdir],
                 ac_cv_func_ftw_working,
  [
  # The following test would fail prior to glibc-2.3.2, because `depth'
  # would be 2 rather than 4.  Of course, now that we require FTW_DCH
  # and FTW_DCHP, this test fails even with GNU libc's fixed ftw.
  mkdir -p conftest.dir/a/b/c
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <string.h>
#include <stdlib.h>
#include <ftw.h>

static char *_f[] = { "conftest.dir", "conftest.dir/a",
		      "conftest.dir/a/b", "conftest.dir/a/b/c" };
static char **p = _f;
static int depth;

static int
cb (const char *file, const struct stat *sb, int file_type, struct FTW *info)
{
  if (strcmp (file, *p++) != 0)
    exit (1);
  ++depth;
  return 0;
}

int
main ()
{
  /* Require these symbols, too.  */
  int d1 = FTW_DCH;
  int d2 = FTW_DCHP;

  int err = nftw ("conftest.dir", cb, 30, FTW_PHYS | FTW_MOUNT | FTW_CHDIR);
  exit ((err == 0 && depth == 4) ? 0 : 1);
}
]])],
               [ac_cv_func_ftw_working=yes],
               [ac_cv_func_ftw_working=no],
               [ac_cv_func_ftw_working=no])])
  rm -rf conftest.dir
  if test $ac_cv_func_ftw_working = no; then
    AC_LIBOBJ([ftw])
    AC_CONFIG_LINKS([$ac_config_libobj_dir/ftw.h:$ac_config_libobj_dir/ftw_.h])
    # Add tsearch.o IFF we have to use the replacement ftw.c.
    if test -n "$need_tdestroy"; then
      AC_LIBOBJ([tsearch])
      # Link search.h to search_.h if we use the replacement tsearch.c.
      AC_CONFIG_LINKS(
        [$ac_config_libobj_dir/search.h:$ac_config_libobj_dir/search_.h])
    fi
  fi
])# AC_FUNC_FTW

#serial 7

dnl From Jim Meyering.
dnl Determine whether lstat has the bug that it succeeds when given the
dnl zero-length file name argument.  The lstat from SunOS4.1.4 and the Hurd
dnl (as of 1998-11-01) do this.
dnl
dnl If it does, then define HAVE_LSTAT_EMPTY_STRING_BUG and arrange to
dnl compile the wrapper function.
dnl

AC_DEFUN([jm_FUNC_LSTAT],
[
 AC_REQUIRE([AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK])
 AC_CACHE_CHECK([whether lstat accepts an empty string],
  jm_cv_func_lstat_empty_string_bug,
  [AC_TRY_RUN([
#   include <sys/types.h>
#   include <sys/stat.h>

    int
    main ()
    {
      struct stat sbuf;
      exit (lstat ("", &sbuf) ? 1 : 0);
    }
	  ],
	 jm_cv_func_lstat_empty_string_bug=yes,
	 jm_cv_func_lstat_empty_string_bug=no,
	 dnl When crosscompiling, assume lstat is broken.
	 jm_cv_func_lstat_empty_string_bug=yes)
  ])
  if test $jm_cv_func_lstat_empty_string_bug = yes; then
    AC_LIBOBJ(lstat)
    AC_DEFINE(HAVE_LSTAT_EMPTY_STRING_BUG, 1,
[Define if lstat has the bug that it succeeds when given the zero-length
   file name argument.  The lstat from SunOS4.1.4 and the Hurd as of 1998-11-01)
   do this. ])
  fi
])

#serial 2

dnl From Jim Meyering.
dnl Provide lchown on systems that lack it.

AC_DEFUN([jm_FUNC_LCHOWN],
[
  AC_REQUIRE([AC_TYPE_UID_T])
  AC_REPLACE_FUNCS(lchown)
])

#serial 3

# When rmdir fails because the specified directory is not empty, it sets
# errno to some value, usually ENOTEMPTY.  However, on some AIX systems,
# ENOTEMPTY is mistakenly defined to be EEXIST.  To work around this, and
# in general, to avoid depending on the use of any particular symbol, this
# test runs a test to determine the actual numeric value.
AC_DEFUN([fetish_FUNC_RMDIR_NOTEMPTY],
[dnl
  AC_CACHE_CHECK([for rmdir-not-empty errno value],
    fetish_cv_func_rmdir_errno_not_empty,
    [
      # Arrange for deletion of the temporary directory this test creates.
      ac_clean_files="$ac_clean_files confdir2"
      mkdir confdir2; : > confdir2/file
      AC_TRY_RUN([
#include <stdio.h>
#include <errno.h>
#ifndef errno
extern int errno;
#endif
	int main ()
	{
	  FILE *s;
	  int val;
	  rmdir ("confdir2");
	  val = errno;
	  s = fopen ("confdir2/errno", "w");
	  fprintf (s, "%d\n", val);
	  exit (0);
	}
	],
      fetish_cv_func_rmdir_errno_not_empty=`cat confdir2/errno`,
      fetish_cv_func_rmdir_errno_not_empty='configure error in rmdir-errno.m4',
      fetish_cv_func_rmdir_errno_not_empty=ENOTEMPTY
      )
    ]
  )

  AC_DEFINE_UNQUOTED([RMDIR_ERRNO_NOT_EMPTY],
    $fetish_cv_func_rmdir_errno_not_empty,
    [the value to which errno is set when rmdir fails on a nonempty directory])
])

#serial 7

dnl From Jim Meyering.
dnl Determine whether chown accepts arguments of -1 for uid and gid.
dnl If it doesn't, arrange to use the replacement function.
dnl

AC_DEFUN([jm_FUNC_CHOWN],
[AC_REQUIRE([AC_TYPE_UID_T])dnl
 test -z "$ac_cv_header_unistd_h" \
   && AC_CHECK_HEADERS(unistd.h)
 AC_CACHE_CHECK([for working chown], jm_cv_func_working_chown,
  [AC_TRY_RUN([
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#   ifdef HAVE_UNISTD_H
#    include <unistd.h>
#   endif

    int
    main ()
    {
      char *f = "conftest.chown";
      struct stat before, after;

      if (creat (f, 0600) < 0)
        exit (1);
      if (stat (f, &before) < 0)
        exit (1);
      if (chown (f, (uid_t) -1, (gid_t) -1) == -1)
        exit (1);
      if (stat (f, &after) < 0)
        exit (1);
      exit ((before.st_uid == after.st_uid
	     && before.st_gid == after.st_gid) ? 0 : 1);
    }
	      ],
	     jm_cv_func_working_chown=yes,
	     jm_cv_func_working_chown=no,
	     dnl When crosscompiling, assume chown is broken.
	     jm_cv_func_working_chown=no)
  ])
  if test $jm_cv_func_working_chown = no; then
    AC_LIBOBJ(chown)
    AC_DEFINE(chown, rpl_chown,
      [Define to rpl_chown if the replacement function should be used.])
  fi
])

#serial 8

dnl From Jim Meyering.
dnl A wrapper around AC_FUNC_MKTIME.

AC_DEFUN([jm_FUNC_MKTIME],
[AC_REQUIRE([AC_FUNC_MKTIME])dnl

 dnl mktime.c uses localtime_r if it exists.  Check for it.
 AC_CHECK_FUNCS(localtime_r)

 if test $ac_cv_func_working_mktime = no; then
   AC_DEFINE(mktime, rpl_mktime,
    [Define to rpl_mktime if the replacement function should be used.])
 fi
])

#serial 7

dnl From Jim Meyering.
dnl Determine whether stat has the bug that it succeeds when given the
dnl zero-length file name argument.  The stat from SunOS4.1.4 and the Hurd
dnl (as of 1998-11-01) do this.
dnl
dnl If it does, then define HAVE_STAT_EMPTY_STRING_BUG and arrange to
dnl compile the wrapper function.
dnl

AC_DEFUN([jm_FUNC_STAT],
[
 AC_REQUIRE([AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK])
 AC_CACHE_CHECK([whether stat accepts an empty string],
  jm_cv_func_stat_empty_string_bug,
  [AC_TRY_RUN([
#   include <sys/types.h>
#   include <sys/stat.h>

    int
    main ()
    {
      struct stat sbuf;
      exit (stat ("", &sbuf) ? 1 : 0);
    }
	  ],
	 jm_cv_func_stat_empty_string_bug=yes,
	 jm_cv_func_stat_empty_string_bug=no,
	 dnl When crosscompiling, assume stat is broken.
	 jm_cv_func_stat_empty_string_bug=yes)
  ])
  if test $jm_cv_func_stat_empty_string_bug = yes; then
    AC_LIBOBJ(stat)
    AC_DEFINE(HAVE_STAT_EMPTY_STRING_BUG, 1,
[Define if stat has the bug that it succeeds when given the zero-length
   file name argument.  The stat from SunOS4.1.4 and the Hurd as of 1998-11-01)
   do this. ])
  fi
])

#serial 8

dnl From Jim Meyering.
dnl Check for the nanosleep function.
dnl If not found, use the supplied replacement.
dnl

AC_DEFUN([jm_FUNC_NANOSLEEP],
[
 nanosleep_save_libs=$LIBS

 # Solaris 2.5.1 needs -lposix4 to get the nanosleep function.
 # Solaris 7 prefers the library name -lrt to the obsolescent name -lposix4.
 AC_SEARCH_LIBS(nanosleep, [rt posix4], [LIB_NANOSLEEP=$ac_cv_search_nanosleep])
 AC_SUBST(LIB_NANOSLEEP)

 AC_CACHE_CHECK([whether nanosleep works],
  jm_cv_func_nanosleep_works,
  [
   AC_REQUIRE([AC_HEADER_TIME])
   AC_TRY_RUN([
#   if TIME_WITH_SYS_TIME
#    include <sys/time.h>
#    include <time.h>
#   else
#    if HAVE_SYS_TIME_H
#     include <sys/time.h>
#    else
#     include <time.h>
#    endif
#   endif

    int
    main ()
    {
      struct timespec ts_sleep, ts_remaining;
      ts_sleep.tv_sec = 0;
      ts_sleep.tv_nsec = 1;
      exit (nanosleep (&ts_sleep, &ts_remaining) == 0 ? 0 : 1);
    }
	  ],
	 jm_cv_func_nanosleep_works=yes,
	 jm_cv_func_nanosleep_works=no,
	 dnl When crosscompiling, assume the worst.
	 jm_cv_func_nanosleep_works=no)
  ])
  if test $jm_cv_func_nanosleep_works = no; then
    AC_LIBOBJ(nanosleep)
    AC_DEFINE(nanosleep, rpl_nanosleep,
      [Define to rpl_nanosleep if the replacement function should be used.])
  fi

 LIBS=$nanosleep_save_libs
])

#serial 7 -*- autoconf -*-

dnl From Jim Meyering.
dnl
dnl See if the glibc *_unlocked I/O macros are available.
dnl Use only those *_unlocked macros that are declared.
dnl

AC_DEFUN([jm_FUNC_GLIBC_UNLOCKED_IO],
  [AC_CHECK_DECLS(
     [clearerr_unlocked, feof_unlocked, ferror_unlocked,
      fflush_unlocked, fgets_unlocked, fputc_unlocked, fputs_unlocked,
      fread_unlocked, fwrite_unlocked, getc_unlocked,
      getchar_unlocked, putc_unlocked, putchar_unlocked])])

#serial 3

dnl Written by Jim Meyering

AC_DEFUN([jm_FUNC_GROUP_MEMBER],
  [
    dnl Do this replacement check manually because I want the hyphen
    dnl (not the underscore) in the filename.
    AC_CHECK_FUNC(group_member, , [AC_LIBOBJ(group-member)])
  ]
)

#serial 5

dnl From Jim Meyering.
dnl
dnl Check whether putenv ("FOO") removes FOO from the environment.
dnl The putenv in libc on at least SunOS 4.1.4 does *not* do that.
dnl

AC_DEFUN([jm_FUNC_PUTENV],
[AC_CACHE_CHECK([for SVID conformant putenv], jm_cv_func_svid_putenv,
  [AC_TRY_RUN([
    int
    main ()
    {
      /* Put it in env.  */
      if (putenv ("CONFTEST_putenv=val"))
        exit (1);

      /* Try to remove it.  */
      if (putenv ("CONFTEST_putenv"))
        exit (1);

      /* Make sure it was deleted.  */
      if (getenv ("CONFTEST_putenv") != 0)
        exit (1);

      exit (0);
    }
	      ],
	     jm_cv_func_svid_putenv=yes,
	     jm_cv_func_svid_putenv=no,
	     dnl When crosscompiling, assume putenv is broken.
	     jm_cv_func_svid_putenv=no)
  ])
  if test $jm_cv_func_svid_putenv = no; then
    AC_LIBOBJ(putenv)
    AC_DEFINE(putenv, rpl_putenv,
      [Define to rpl_putenv if the replacement function should be used.])
  fi
])

#serial 5

AC_DEFUN([jm_AFS],
  [
    AC_MSG_CHECKING(for AFS)
    if test -d /afs; then
      AC_DEFINE(AFS, 1, [Define if you have the Andrew File System.])
      ac_result=yes
    else
      ac_result=no
    fi
    AC_MSG_RESULT($ac_result)
  ])

#serial 4

# autoconf tests required for use of xstrtoumax.c

AC_DEFUN([jm_AC_PREREQ_XSTRTOUMAX],
[
  AC_REQUIRE([jm_AC_TYPE_INTMAX_T])
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
  AC_REQUIRE([jm_AC_TYPE_LONG_LONG])
  AC_REQUIRE([jm_AC_TYPE_UNSIGNED_LONG_LONG])
  AC_CHECK_DECLS([strtol, strtoul, strtoull, strtoimax, strtoumax])
  AC_CHECK_HEADERS(limits.h stdlib.h inttypes.h)

  AC_CACHE_CHECK([whether <inttypes.h> defines strtoumax as a macro],
    jm_cv_func_strtoumax_macro,
    AC_EGREP_CPP([inttypes_h_defines_strtoumax], [#include <inttypes.h>
#ifdef strtoumax
 inttypes_h_defines_strtoumax
#endif],
      jm_cv_func_strtoumax_macro=yes,
      jm_cv_func_strtoumax_macro=no))

  if test "$jm_cv_func_strtoumax_macro" != yes; then
    AC_REPLACE_FUNCS(strtoumax)
  fi

  dnl Only the replacement strtoumax invokes strtoul and strtoull,
  dnl so we need the replacements only if strtoumax does not exist.
  case "$jm_cv_func_strtoumax_macro,$ac_cv_func_strtoumax" in
    no,no)
      AC_REPLACE_FUNCS(strtoul)

      dnl We don't need (and can't compile) the replacement strtoull
      dnl unless the type `unsigned long long' exists.
      if test "$ac_cv_type_unsigned_long_long" = yes; then
	AC_REPLACE_FUNCS(strtoull)
      fi
      ;;
  esac
])

#serial 2
dnl Cloned from xstrtoumax.m4.  Keep these files in sync.

# autoconf tests required for use of xstrtoimax.c

AC_DEFUN([jm_AC_PREREQ_XSTRTOIMAX],
[
  AC_REQUIRE([jm_AC_TYPE_INTMAX_T])
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
  AC_REQUIRE([jm_AC_TYPE_LONG_LONG])
  AC_REQUIRE([jm_AC_TYPE_UNSIGNED_LONG_LONG])
  AC_CHECK_DECLS([strtol, strtoul, strtoll, strtoimax, strtoumax])
  AC_CHECK_HEADERS(limits.h stdlib.h inttypes.h)

  AC_CACHE_CHECK([whether <inttypes.h> defines strtoimax as a macro],
    jm_cv_func_strtoimax_macro,
    AC_EGREP_CPP([inttypes_h_defines_strtoimax], [#include <inttypes.h>
#ifdef strtoimax
 inttypes_h_defines_strtoimax
#endif],
      jm_cv_func_strtoimax_macro=yes,
      jm_cv_func_strtoimax_macro=no))

  if test "$jm_cv_func_strtoimax_macro" != yes; then
    AC_REPLACE_FUNCS(strtoimax)
  fi

  dnl Only the replacement strtoimax invokes strtol and strtoll,
  dnl so we need the replacements only if strtoimax does not exist.
  case "$jm_cv_func_strtoimax_macro,$ac_cv_func_strtoimax" in
    no,no)
      AC_REPLACE_FUNCS(strtol)

      dnl We don't need (and can't compile) the replacement strtoll
      dnl unless the type `long long' exists.
      if test "$ac_cv_type_long_long" = yes; then
	AC_REPLACE_FUNCS(strtoll)
      fi
      ;;
  esac
])

#serial 3
dnl Run a program to determine whether whether link(2) follows symlinks.
dnl Set LINK_FOLLOWS_SYMLINKS accordingly.

AC_DEFUN([jm_AC_FUNC_LINK_FOLLOWS_SYMLINK],
[dnl
  AC_CACHE_CHECK(
    [whether link(2) dereferences a symlink specified with a trailing slash],
		 jm_ac_cv_func_link_follows_symlink,
  [
    dnl poor-man's AC_REQUIRE: FIXME: repair this once autoconf-3 provides
    dnl the appropriate framework.
    test -z "$ac_cv_header_unistd_h" \
      && AC_CHECK_HEADERS(unistd.h)

    # Create a regular file.
    echo > conftest.file
    AC_TRY_RUN(
      [
#       include <sys/types.h>
#       include <sys/stat.h>
#       ifdef HAVE_UNISTD_H
#        include <unistd.h>
#       endif

#       define SAME_INODE(Stat_buf_1, Stat_buf_2) \
	  ((Stat_buf_1).st_ino == (Stat_buf_2).st_ino \
	   && (Stat_buf_1).st_dev == (Stat_buf_2).st_dev)

	int
	main ()
	{
	  const char *file = "conftest.file";
	  const char *sym = "conftest.sym";
	  const char *hard = "conftest.hard";
	  struct stat sb_file, sb_hard;

	  /* Create a symlink to the regular file. */
	  if (symlink (file, sym))
	    abort ();

	  /* Create a hard link to that symlink.  */
	  if (link (sym, hard))
	    abort ();

	  if (lstat (hard, &sb_hard))
	    abort ();
	  if (lstat (file, &sb_file))
	    abort ();

	  /* If the dev/inode of hard and file are the same, then
	     the link call followed the symlink.  */
	  return SAME_INODE (sb_hard, sb_file) ? 0 : 1;
	}
      ],
      jm_ac_cv_func_link_follows_symlink=yes,
      jm_ac_cv_func_link_follows_symlink=no,
      jm_ac_cv_func_link_follows_symlink=yes dnl We're cross compiling.
    )
  ])
  if test $jm_ac_cv_func_link_follows_symlink = yes; then
    AC_DEFINE(LINK_FOLLOWS_SYMLINKS, 1,
      [Define if `link(2)' dereferences symbolic links.])
  fi
])

#serial 17

dnl This macro is intended to be used solely in this file.
dnl These are the prerequisite macros for GNU's strftime.c replacement.
AC_DEFUN([_jm_STRFTIME_PREREQS],
[
 dnl strftime.c uses localtime_r and the underyling system strftime
 dnl if they exist.
 AC_CHECK_FUNCS(localtime_r strftime)

 AC_CHECK_HEADERS(limits.h)
 AC_CHECK_FUNCS(bcopy tzset mempcpy memcpy memset)

 # This defines (or not) HAVE_TZNAME and HAVE_TM_ZONE.
 AC_STRUCT_TIMEZONE

 AC_CHECK_FUNCS(mblen mbrlen)

 AC_CHECK_MEMBER([struct tm.tm_gmtoff],
                 [AC_DEFINE(HAVE_TM_GMTOFF, 1,
                            [Define if struct tm has the tm_gmtoff member.])],
                 ,
                 [#include <time.h>])
])

dnl From Jim Meyering.
dnl
AC_DEFUN([jm_FUNC_GNU_STRFTIME],
[AC_REQUIRE([AC_HEADER_TIME])dnl

 _jm_STRFTIME_PREREQS

 AC_REQUIRE([AC_C_CONST])dnl
 AC_REQUIRE([AC_HEADER_STDC])dnl
 AC_CHECK_HEADERS(sys/time.h)
 AC_DEFINE([my_strftime], [nstrftime],
   [Define to the name of the strftime replacement function.])
])

AC_DEFUN([jm_FUNC_STRFTIME],
[
  _jm_STRFTIME_PREREQS
])

#serial 3

dnl From Jim Meyering
dnl Using code from emacs, based on suggestions from Paul Eggert
dnl and Ulrich Drepper.

dnl Find out how to determine the number of pending output bytes on a stream.
dnl glibc (2.1.93 and newer) and Solaris provide __fpending.  On other systems,
dnl we have to grub around in the FILE struct.

AC_DEFUN([jm_FUNC_FPENDING],
[
  AC_CHECK_HEADERS(stdio_ext.h)
  AC_REPLACE_FUNCS([__fpending])
  fp_headers='
#     if HAVE_STDIO_EXT_H
#      include <stdio_ext.h>
#     endif
'
  AC_CHECK_DECLS([__fpending], , , $fp_headers)
  if test $ac_cv_func___fpending = no; then
    AC_CACHE_CHECK(
	      [how to determine the number of pending output bytes on a stream],
		   ac_cv_sys_pending_output_n_bytes,
      [
	for ac_expr in						\
								\
	    '# glibc2'						\
	    'fp->_IO_write_ptr - fp->_IO_write_base'		\
								\
	    '# traditional Unix'				\
	    'fp->_ptr - fp->_base'				\
								\
	    '# BSD'						\
	    'fp->_p - fp->_bf._base'				\
								\
	    '# SCO, Unixware'					\
	    'fp->__ptr - fp->__base'				\
								\
	    '# old glibc?'					\
	    'fp->__bufp - fp->__buffer'				\
								\
	    '# old glibc iostream?'				\
	    'fp->_pptr - fp->_pbase'				\
								\
	    '# VMS'						\
	    '(*fp)->_ptr - (*fp)->_base'			\
								\
	    '# e.g., DGUX R4.11; the info is not available'	\
	    1							\
	    ; do

	  # Skip each embedded comment.
	  case "$ac_expr" in '#'*) continue;; esac

	  AC_TRY_COMPILE(
	    [#include <stdio.h>
	    ],
	    [FILE *fp = stdin; (void) ($ac_expr);],
	    fp_done=yes
	  )
	  test "$fp_done" = yes && break
	done

	ac_cv_sys_pending_output_n_bytes=$ac_expr
      ]
    )
    AC_DEFINE_UNQUOTED(PENDING_OUTPUT_N_BYTES,
      $ac_cv_sys_pending_output_n_bytes,
      [the number of pending output bytes on stream `fp'])
  fi
])

# inttypes-pri.m4 serial 1001 (based on gettext-0.11.4's `serial 1')
dnl Copyright (C) 1997-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl From Bruno Haible.

# Define PRI_MACROS_BROKEN if <inttypes.h> exists and defines the PRI*
# macros to non-string values.  This is the case on AIX 4.3.3.

AC_DEFUN([gt_INTTYPES_PRI],
[
  # autoconf-2.52 has a proper check for inttypes.h.
  AC_PREREQ(2.52)

  if test $ac_cv_header_inttypes_h = yes; then
    AC_CACHE_CHECK([whether the inttypes.h PRIxNN macros are broken],
      gt_cv_inttypes_pri_broken,
      [
        AC_TRY_COMPILE([#include <inttypes.h>
#ifdef PRId32
char *p = PRId32;
#endif
], [], gt_cv_inttypes_pri_broken=no, gt_cv_inttypes_pri_broken=yes)
      ])
  fi
  if test "$gt_cv_inttypes_pri_broken" = yes; then
    AC_DEFINE_UNQUOTED(PRI_MACROS_BROKEN, 1,
      [Define if <inttypes.h> exists and defines unusable PRI* macros.])
  fi
])

#serial 7

dnl From Jim Meyering.
dnl A wrapper around AC_FUNC_GETGROUPS.

AC_DEFUN([jm_FUNC_GETGROUPS],
[AC_REQUIRE([AC_FUNC_GETGROUPS])dnl
  if test $ac_cv_func_getgroups_works = no; then
    AC_LIBOBJ(getgroups)
    AC_DEFINE(getgroups, rpl_getgroups,
      [Define as rpl_getgroups if getgroups doesn't work right.])
  fi
  test -n "$GETGROUPS_LIB" && LIBS="$GETGROUPS_LIB $LIBS"
])

#serial 5

AC_PREREQ(2.13)

AC_DEFUN([jm_SYS_PROC_UPTIME],
[ dnl Require AC_PROG_CC to see if we're cross compiling.
  AC_REQUIRE([AC_PROG_CC])
  AC_CACHE_CHECK([for /proc/uptime], jm_cv_have_proc_uptime,
  [jm_cv_have_proc_uptime=no
    test -f /proc/uptime \
      && test "$cross_compiling" = no \
      && cat < /proc/uptime >/dev/null 2>/dev/null \
      && jm_cv_have_proc_uptime=yes])
  if test $jm_cv_have_proc_uptime = yes; then
    AC_DEFINE(HAVE_PROC_UPTIME, 1,
	      [  Define if your system has the /proc/uptime special file.])
  fi
])

#serial 4

# See if we need to emulate a missing ftruncate function using fcntl or chsize.

AC_DEFUN([jm_FUNC_FTRUNCATE],
[
  AC_CHECK_FUNCS(ftruncate, , [ftruncate_missing=yes])

  if test "$ftruncate_missing" = yes; then
    AC_CHECK_HEADERS([unistd.h])
    AC_CHECK_FUNCS([chsize])
    AC_LIBOBJ(ftruncate)
  fi
])

#serial 3

dnl From Volker Borchert.
dnl Determine whether rename works for source paths with a trailing slash.
dnl The rename from SunOS 4.1.1_U1 doesn't.
dnl
dnl If it doesn't, then define RENAME_TRAILING_SLASH_BUG and arrange
dnl to compile the wrapper function.
dnl

AC_DEFUN([vb_FUNC_RENAME],
[
 AC_CACHE_CHECK([whether rename is broken],
  vb_cv_func_rename_trailing_slash_bug,
  [
    rm -rf conftest.d1 conftest.d2
    mkdir conftest.d1 ||
      AC_MSG_ERROR([cannot create temporary directory])
    AC_TRY_RUN([
#       include <stdio.h>
        int
        main ()
        {
          exit (rename ("conftest.d1/", "conftest.d2") ? 1 : 0);
        }
      ],
      vb_cv_func_rename_trailing_slash_bug=no,
      vb_cv_func_rename_trailing_slash_bug=yes,
      dnl When crosscompiling, assume rename is broken.
      vb_cv_func_rename_trailing_slash_bug=yes)

      rm -rf conftest.d1 conftest.d2
  ])
  if test $vb_cv_func_rename_trailing_slash_bug = yes; then
    AC_LIBOBJ(rename)
    AC_DEFINE(RENAME_TRAILING_SLASH_BUG, 1,
[Define if rename does not work for source paths with a trailing slash,
   like the one from SunOS 4.1.1_U1.])
  fi
])

#serial 1

# On some systems (e.g., HPUX-10.20, SunOS4.1.4, solaris2.5.1), mkstemp has the
# silly limit that it can create no more than 26 files from a given template.
# Other systems lack mkstemp altogether.  On either type of system, arrange
# to use the replacement function.
AC_DEFUN([UTILS_FUNC_MKSTEMP],
[dnl
  AC_REPLACE_FUNCS(mkstemp)
  if test $ac_cv_func_mkstemp = no; then
    utils_cv_func_mkstemp_limitations=yes
  else
    AC_CACHE_CHECK([for mkstemp limitations],
      utils_cv_func_mkstemp_limitations,
      [
	AC_TRY_RUN([
#         include <stdlib.h>
	  int main ()
	  {
	    int i;
	    for (i = 0; i < 30; i++)
	      {
		char template[] = "conftestXXXXXX";
		int fd = mkstemp (template);
		if (fd == -1)
		  exit (1);
		close (fd);
	      }
	    exit (0);
	  }
	  ],
	utils_cv_func_mkstemp_limitations=no,
	utils_cv_func_mkstemp_limitations=yes,
	utils_cv_func_mkstemp_limitations=yes
	)
      ]
    )
  fi

  if test $utils_cv_func_mkstemp_limitations = yes; then
    AC_LIBOBJ(mkstemp)
    AC_LIBOBJ(tempname)
    AC_DEFINE(mkstemp, rpl_mkstemp,
      [Define to rpl_mkstemp if the replacement function should be used.])
  fi
])

#serial 2

dnl From Jim Meyering.
dnl
dnl See if gettimeofday clobbers the static buffer that localtime uses
dnl for it's return value.  The gettimeofday function from Mac OS X 10.0.4,
dnl i.e. Darwin 1.3.7 has this problem.
dnl
dnl If it does, then arrange to use gettimeofday and localtime only via
dnl the wrapper functions that work around the problem.

AC_DEFUN([AC_FUNC_GETTIMEOFDAY_CLOBBER],
[
 AC_REQUIRE([AC_HEADER_TIME])
 AC_CHECK_HEADERS(string.h stdlib.h)
 AC_CACHE_CHECK([whether gettimeofday clobbers localtime buffer],
  jm_cv_func_gettimeofday_clobber,
  [AC_TRY_RUN([
#include <stdio.h>
#if HAVE_STRING_H
# include <string.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif

int
main ()
{
  time_t t = 0;
  struct tm *lt;
  struct tm saved_lt;
  struct timeval tv;
  lt = localtime (&t);
  saved_lt = *lt;
  gettimeofday (&tv, NULL);
  if (memcmp (lt, &saved_lt, sizeof (struct tm)) != 0)
    exit (1);

  exit (0);
}
	  ],
	 jm_cv_func_gettimeofday_clobber=no,
	 jm_cv_func_gettimeofday_clobber=yes,
	 dnl When crosscompiling, assume it is broken.
	 jm_cv_func_gettimeofday_clobber=yes)
  ])
  if test $jm_cv_func_gettimeofday_clobber = yes; then
    AC_LIBOBJ(gettimeofday)
    AC_DEFINE(localtime, rpl_localtime,
      [Define to rpl_localtime if the replacement function should be used.])
    AC_DEFINE(gettimeofday, rpl_gettimeofday,
      [Define to rpl_gettimeofday if the replacement function should be used.])
    AC_DEFINE(GETTIMEOFDAY_CLOBBERS_LOCALTIME_BUFFER, 1,
      [Define if gettimeofday clobbers localtime's static buffer.])
  fi
])

#serial 3

dnl From Jim Meyering
dnl Replace the utime function on systems that need it.

dnl FIXME

AC_DEFUN([jm_FUNC_UTIME],
[
  AC_CHECK_HEADERS(utime.h)
  AC_REQUIRE([jm_CHECK_TYPE_STRUCT_UTIMBUF])
  AC_REQUIRE([AC_FUNC_UTIME_NULL])

  if test $ac_cv_func_utime_null = no; then
    jm_FUNC_UTIMES_NULL
    AC_REPLACE_FUNCS(utime)
  fi
])

#serial 4

dnl Shamelessly cloned from acspecific.m4's AC_FUNC_UTIME_NULL,
dnl then do case-insensitive s/utime/utimes/.

AC_DEFUN([jm_FUNC_UTIMES_NULL],
[AC_CACHE_CHECK(whether utimes accepts a null argument, ac_cv_func_utimes_null,
[rm -f conftest.data; > conftest.data
AC_TRY_RUN([
/* In case stat has been defined to rpl_stat, undef it here.  */
#undef stat
#include <sys/types.h>
#include <sys/stat.h>
main() {
struct stat s, t;
exit(!(stat ("conftest.data", &s) == 0
       && utimes("conftest.data", (long *)0) == 0
       && stat("conftest.data", &t) == 0
       && t.st_mtime >= s.st_mtime
       && t.st_mtime - s.st_mtime < 120));
}],
  ac_cv_func_utimes_null=yes,
  ac_cv_func_utimes_null=no,
  ac_cv_func_utimes_null=no)
rm -f core core.* *.core])

    if test $ac_cv_func_utimes_null = yes; then
      AC_DEFINE(HAVE_UTIMES_NULL, 1,
		[Define if utimes accepts a null argument])
    fi
  ]
)

#serial 5

dnl See if there's a working, system-supplied version of the getline function.
dnl We can't just do AC_REPLACE_FUNCS(getline) because some systems
dnl have a function by that name in -linet that doesn't have anything
dnl to do with the function we need.
AC_DEFUN([AM_FUNC_GETLINE],
[dnl
  am_getline_needs_run_time_check=no
  AC_CHECK_FUNC(getline,
		dnl Found it in some library.  Verify that it works.
		am_getline_needs_run_time_check=yes,
		am_cv_func_working_getline=no)
  if test $am_getline_needs_run_time_check = yes; then
    AC_CACHE_CHECK([for working getline function], am_cv_func_working_getline,
    [echo fooN |tr -d '\012'|tr N '\012' > conftest.data
    AC_TRY_RUN([
#    include <stdio.h>
#    include <sys/types.h>
#    include <string.h>
    int main ()
    { /* Based on a test program from Karl Heuer.  */
      char *line = NULL;
      size_t siz = 0;
      int len;
      FILE *in = fopen ("./conftest.data", "r");
      if (!in)
	return 1;
      len = getline (&line, &siz, in);
      exit ((len == 4 && line && strcmp (line, "foo\n") == 0) ? 0 : 1);
    }
    ], am_cv_func_working_getline=yes dnl The library version works.
    , am_cv_func_working_getline=no dnl The library version does NOT work.
    , am_cv_func_working_getline=no dnl We're cross compiling.
    )])
  fi

  if test $am_cv_func_working_getline = no; then
    AC_LIBOBJ(getline)
  fi
])

#serial 1
# Determine approximately how many files may be open simultaneously
# in one process.  This is approximate, since while running this test,
# the configure script already has a few files open.
# From Jim Meyering

AC_DEFUN([UTILS_SYS_OPEN_MAX],
[
  AC_CACHE_CHECK([determine how many files may be open simultaneously],
                 utils_cv_sys_open_max,
  [
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
    int
    main ()
    {
      FILE *result = fopen ("conftest.omax", "w");
      int i = 1;
      /* Impose an arbitrary limit, in case some system has no
	 effective limit on the number of simultaneously open files.  */
      while (i < 30000)
	{
	  FILE *s = fopen ("conftest.op", "w");
	  if (!s)
	    break;
	  ++i;
	}
      fprintf (result, "%d\n", i);
      exit (fclose (result) == EOF);
    }
  ]])],
       [utils_cv_sys_open_max=`cat conftest.omax`],
       [utils_cv_sys_open_max='internal error in open-max.m4'],
       [utils_cv_sys_open_max='cross compiling run-test in open-max.m4'])])

  AC_DEFINE_UNQUOTED([UTILS_OPEN_MAX],
    $utils_cv_sys_open_max,
    [the maximum number of simultaneously open files per process])
])

#serial 2
# Check whether getcwd has the bug that it succeeds for a working directory
# longer than PATH_MAX, yet returns a truncated directory name.
# If so, arrange to compile the wrapper function.

# This is necessary for at least GNU libc on linux-2.4.19 and 2.4.20.
# I've heard that this is due to a Linux kernel bug, and that it has
# been fixed between 2.4.21-pre3 and 2.4.21-pre4.  */

# From Jim Meyering

AC_DEFUN([GL_FUNC_GETCWD_PATH_MAX],
[
  AC_CACHE_CHECK([whether getcwd properly handles paths longer than PATH_MAX],
                 gl_cv_func_getcwd_vs_path_max,
  [
  AC_CHECK_DECLS([getcwd])
  # Arrange for deletion of the temporary directory this test creates.
  ac_clean_files="$ac_clean_files confdir3"
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Don't get link errors because mkdir is redefined to rpl_mkdir.  */
#undef mkdir

#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif

/* The extra casts work around common compiler bugs.  */
#define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))
/* The outer cast is needed to work around a bug in Cray C 5.0.3.0.
   It is necessary at least when t == time_t.  */
#define TYPE_MINIMUM(t) ((t) (TYPE_SIGNED (t) \
			      ? ~ (t) 0 << (sizeof (t) * CHAR_BIT - 1) : (t) 0))
#define TYPE_MAXIMUM(t) ((t) (~ (t) 0 - TYPE_MINIMUM (t)))

#ifndef INT_MAX
# define INT_MAX TYPE_MAXIMUM (int)
#endif

#ifndef PATH_MAX
/* There might be a better way to handle this case, but note:
   - the value shouldn't be anywhere near INT_MAX, and
   - the value shouldn't be so big that the local declaration, below,
   blows the stack.  */
# define PATH_MAX 40000
#endif

/* The length of this name must be 8.  */
#define DIR_NAME "confdir3"

int
main ()
{
  /* The '9' comes from strlen (DIR_NAME) + 1.  */
#if INT_MAX - 9 <= PATH_MAX
  /* FIXME: Assuming there's a system for which this is true -- Hurd?,
     this should be done in a compile test.  */
  exit (0);
#else
  char buf[PATH_MAX + 20];
  char *cwd = getcwd (buf, PATH_MAX);
  size_t cwd_len;
  int fail = 0;
  size_t n_chdirs = 0;

  if (cwd == NULL)
    exit (1);

  cwd_len = strlen (cwd);

  while (1)
    {
      char *c;
      size_t len;

      cwd_len += 1 + strlen (DIR_NAME);
      /* If mkdir or chdir fails, be pessimistic and consider that
	 as a failure, too.  */
      if (mkdir (DIR_NAME, 0700) < 0 || chdir (DIR_NAME) < 0)
	{
	  fail = 1;
	  break;
	}
      if ((c = getcwd (buf, PATH_MAX)) == NULL)
        {
	  /* This allows any failure to indicate there is no bug.
	     FIXME: check errno?  */
	  break;
	}
      if ((len = strlen (c)) != cwd_len)
	{
	  fail = 1;
	  break;
	}
      ++n_chdirs;
      if (PATH_MAX < len)
	break;
    }

  /* Leaving behind such a deep directory is not polite.
     So clean up here, right away, even though the driving
     shell script would also clean up.  */
  {
    size_t i;

    /* Unlink first, in case the chdir failed.  */
    unlink (DIR_NAME);
    for (i = 0; i <= n_chdirs; i++)
      {
	if (chdir ("..") < 0)
	  break;
	rmdir (DIR_NAME);
      }
  }

  exit (fail);
#endif
}
  ]])],
       [gl_cv_func_getcwd_vs_path_max=yes],
       [gl_cv_func_getcwd_vs_path_max=no],
       [gl_cv_func_getcwd_vs_path_max=no])])

  if test $gl_cv_func_getcwd_vs_path_max = yes; then
    AC_LIBOBJ(getcwd)
    AC_DEFINE(getcwd, rpl_getcwd,
      [Define to rpl_getcwd if the wrapper function should be used.])
  fi
])

# codeset.m4 serial AM1 (gettext-0.10.40)
dnl Copyright (C) 2000-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl From Bruno Haible.

AC_DEFUN([AM_LANGINFO_CODESET],
[
  AC_CACHE_CHECK([for nl_langinfo and CODESET], am_cv_langinfo_codeset,
    [AC_TRY_LINK([#include <langinfo.h>],
      [char* cs = nl_langinfo(CODESET);],
      am_cv_langinfo_codeset=yes,
      am_cv_langinfo_codeset=no)
    ])
  if test $am_cv_langinfo_codeset = yes; then
    AC_DEFINE(HAVE_LANGINFO_CODESET, 1,
      [Define if you have <langinfo.h> and nl_langinfo(CODESET).])
  fi
])

# glibc21.m4 serial 2 (fileutils-4.1.3, gettext-0.10.40)
dnl Copyright (C) 2000-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

# Test for the GNU C Library, version 2.1 or newer.
# From Bruno Haible.

AC_DEFUN([jm_GLIBC21],
  [
    AC_CACHE_CHECK(whether we are using the GNU C Library 2.1 or newer,
      ac_cv_gnu_library_2_1,
      [AC_EGREP_CPP([Lucky GNU user],
	[
#include <features.h>
#ifdef __GNU_LIBRARY__
 #if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1) || (__GLIBC__ > 2)
  Lucky GNU user
 #endif
#endif
	],
	ac_cv_gnu_library_2_1=yes,
	ac_cv_gnu_library_2_1=no)
      ]
    )
    AC_SUBST(GLIBC21)
    GLIBC21="$ac_cv_gnu_library_2_1"
  ]
)

# iconv.m4 serial AM4 (gettext-0.11.3)
dnl Copyright (C) 2000-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl From Bruno Haible.

AC_DEFUN([AM_ICONV_LINKFLAGS_BODY],
[
  dnl Prerequisites of AC_LIB_LINKFLAGS_BODY.
  AC_REQUIRE([AC_LIB_PREPARE_PREFIX])
  AC_REQUIRE([AC_LIB_RPATH])

  dnl Search for libiconv and define LIBICONV, LTLIBICONV and INCICONV
  dnl accordingly.
  AC_LIB_LINKFLAGS_BODY([iconv])
])

AC_DEFUN([AM_ICONV_LINK],
[
  dnl Some systems have iconv in libc, some have it in libiconv (OSF/1 and
  dnl those with the standalone portable GNU libiconv installed).

  dnl Search for libiconv and define LIBICONV, LTLIBICONV and INCICONV
  dnl accordingly.
  AC_REQUIRE([AM_ICONV_LINKFLAGS_BODY])

  dnl Add $INCICONV to CPPFLAGS before performing the following checks,
  dnl because if the user has installed libiconv and not disabled its use
  dnl via --without-libiconv-prefix, he wants to use it. The first
  dnl AC_TRY_LINK will then fail, the second AC_TRY_LINK will succeed.
  am_save_CPPFLAGS="$CPPFLAGS"
  AC_LIB_APPENDTOVAR([CPPFLAGS], [$INCICONV])

  AC_CACHE_CHECK(for iconv, am_cv_func_iconv, [
    am_cv_func_iconv="no, consider installing GNU libiconv"
    am_cv_lib_iconv=no
    AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
      [iconv_t cd = iconv_open("","");
       iconv(cd,NULL,NULL,NULL,NULL);
       iconv_close(cd);],
      am_cv_func_iconv=yes)
    if test "$am_cv_func_iconv" != yes; then
      am_save_LIBS="$LIBS"
      LIBS="$LIBS $LIBICONV"
      AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
        [iconv_t cd = iconv_open("","");
         iconv(cd,NULL,NULL,NULL,NULL);
         iconv_close(cd);],
        am_cv_lib_iconv=yes
        am_cv_func_iconv=yes)
      LIBS="$am_save_LIBS"
    fi
  ])
  if test "$am_cv_func_iconv" = yes; then
    AC_DEFINE(HAVE_ICONV, 1, [Define if you have the iconv() function.])
  fi
  if test "$am_cv_lib_iconv" = yes; then
    AC_MSG_CHECKING([how to link with libiconv])
    AC_MSG_RESULT([$LIBICONV])
  else
    dnl If $LIBICONV didn't lead to a usable library, we don't need $INCICONV
    dnl either.
    CPPFLAGS="$am_save_CPPFLAGS"
    LIBICONV=
    LTLIBICONV=
  fi
  AC_SUBST(LIBICONV)
  AC_SUBST(LTLIBICONV)
])

AC_DEFUN([AM_ICONV],
[
  AM_ICONV_LINK
  if test "$am_cv_func_iconv" = yes; then
    AC_MSG_CHECKING([for iconv declaration])
    AC_CACHE_VAL(am_cv_proto_iconv, [
      AC_TRY_COMPILE([
#include <stdlib.h>
#include <iconv.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
#else
size_t iconv();
#endif
], [], am_cv_proto_iconv_arg1="", am_cv_proto_iconv_arg1="const")
      am_cv_proto_iconv="extern size_t iconv (iconv_t cd, $am_cv_proto_iconv_arg1 char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);"])
    am_cv_proto_iconv=`echo "[$]am_cv_proto_iconv" | tr -s ' ' | sed -e 's/( /(/'`
    AC_MSG_RESULT([$]{ac_t:-
         }[$]am_cv_proto_iconv)
    AC_DEFINE_UNQUOTED(ICONV_CONST, $am_cv_proto_iconv_arg1,
      [Define as const if the declaration of iconv() needs const.])
  fi
])

# lib-prefix.m4 serial 1 (gettext-0.11)
dnl Copyright (C) 2001-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl From Bruno Haible.

dnl AC_LIB_PREFIX adds to the CPPFLAGS and LDFLAGS the flags that are needed
dnl to access previously installed libraries. The basic assumption is that
dnl a user will want packages to use other packages he previously installed
dnl with the same --prefix option.
dnl This macro is not needed if only AC_LIB_LINKFLAGS is used to locate
dnl libraries, but is otherwise very convenient.
AC_DEFUN([AC_LIB_PREFIX],
[
  AC_BEFORE([$0], [AC_LIB_LINKFLAGS])
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_REQUIRE([AC_LIB_PREPARE_PREFIX])
  dnl By default, look in $includedir and $libdir.
  use_additional=yes
  AC_LIB_WITH_FINAL_PREFIX([
    eval additional_includedir=\"$includedir\"
    eval additional_libdir=\"$libdir\"
  ])
  AC_ARG_WITH([lib-prefix],
[  --with-lib-prefix[=DIR] search for libraries in DIR/include and DIR/lib
  --without-lib-prefix    don't search for libraries in includedir and libdir],
[
    if test "X$withval" = "Xno"; then
      use_additional=no
    else
      if test "X$withval" = "X"; then
        AC_LIB_WITH_FINAL_PREFIX([
          eval additional_includedir=\"$includedir\"
          eval additional_libdir=\"$libdir\"
        ])
      else
        additional_includedir="$withval/include"
        additional_libdir="$withval/lib"
      fi
    fi
])
  if test $use_additional = yes; then
    dnl Potentially add $additional_includedir to $CPPFLAGS.
    dnl But don't add it
    dnl   1. if it's the standard /usr/include,
    dnl   2. if it's already present in $CPPFLAGS,
    dnl   3. if it's /usr/local/include and we are using GCC on Linux,
    dnl   4. if it doesn't exist as a directory.
    if test "X$additional_includedir" != "X/usr/include"; then
      haveit=
      for x in $CPPFLAGS; do
        AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
        if test "X$x" = "X-I$additional_includedir"; then
          haveit=yes
          break
        fi
      done
      if test -z "$haveit"; then
        if test "X$additional_includedir" = "X/usr/local/include"; then
          if test -n "$GCC"; then
            case $host_os in
              linux*) haveit=yes;;
            esac
          fi
        fi
        if test -z "$haveit"; then
          if test -d "$additional_includedir"; then
            dnl Really add $additional_includedir to $CPPFLAGS.
            CPPFLAGS="${CPPFLAGS}${CPPFLAGS:+ }-I$additional_includedir"
          fi
        fi
      fi
    fi
    dnl Potentially add $additional_libdir to $LDFLAGS.
    dnl But don't add it
    dnl   1. if it's the standard /usr/lib,
    dnl   2. if it's already present in $LDFLAGS,
    dnl   3. if it's /usr/local/lib and we are using GCC on Linux,
    dnl   4. if it doesn't exist as a directory.
    if test "X$additional_libdir" != "X/usr/lib"; then
      haveit=
      for x in $LDFLAGS; do
        AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
        if test "X$x" = "X-L$additional_libdir"; then
          haveit=yes
          break
        fi
      done
      if test -z "$haveit"; then
        if test "X$additional_libdir" = "X/usr/local/lib"; then
          if test -n "$GCC"; then
            case $host_os in
              linux*) haveit=yes;;
            esac
          fi
        fi
        if test -z "$haveit"; then
          if test -d "$additional_libdir"; then
            dnl Really add $additional_libdir to $LDFLAGS.
            LDFLAGS="${LDFLAGS}${LDFLAGS:+ }-L$additional_libdir"
          fi
        fi
      fi
    fi
  fi
])

dnl AC_LIB_PREPARE_PREFIX creates variables acl_final_prefix,
dnl acl_final_exec_prefix, containing the values to which $prefix and
dnl $exec_prefix will expand at the end of the configure script.
AC_DEFUN([AC_LIB_PREPARE_PREFIX],
[
  dnl Unfortunately, prefix and exec_prefix get only finally determined
  dnl at the end of configure.
  if test "X$prefix" = "XNONE"; then
    acl_final_prefix="$ac_default_prefix"
  else
    acl_final_prefix="$prefix"
  fi
  if test "X$exec_prefix" = "XNONE"; then
    acl_final_exec_prefix='${prefix}'
  else
    acl_final_exec_prefix="$exec_prefix"
  fi
  acl_save_prefix="$prefix"
  prefix="$acl_final_prefix"
  eval acl_final_exec_prefix=\"$acl_final_exec_prefix\"
  prefix="$acl_save_prefix"
])

dnl AC_LIB_WITH_FINAL_PREFIX([statement]) evaluates statement, with the
dnl variables prefix and exec_prefix bound to the values they will have
dnl at the end of the configure script.
AC_DEFUN([AC_LIB_WITH_FINAL_PREFIX],
[
  acl_save_prefix="$prefix"
  prefix="$acl_final_prefix"
  acl_save_exec_prefix="$exec_prefix"
  exec_prefix="$acl_final_exec_prefix"
  $1
  exec_prefix="$acl_save_exec_prefix"
  prefix="$acl_save_prefix"
])

# lib-link.m4 serial 3 (gettext-0.11.3)
dnl Copyright (C) 2001-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl From Bruno Haible.

dnl AC_LIB_LINKFLAGS(name [, dependencies]) searches for libname and
dnl the libraries corresponding to explicit and implicit dependencies.
dnl Sets and AC_SUBSTs the LIB${NAME} and LTLIB${NAME} variables and
dnl augments the CPPFLAGS variable.
AC_DEFUN([AC_LIB_LINKFLAGS],
[
  AC_REQUIRE([AC_LIB_PREPARE_PREFIX])
  AC_REQUIRE([AC_LIB_RPATH])
  define([Name],[translit([$1],[./-], [___])])
  define([NAME],[translit([$1],[abcdefghijklmnopqrstuvwxyz./-],
                               [ABCDEFGHIJKLMNOPQRSTUVWXYZ___])])
  AC_CACHE_CHECK([how to link with lib[]$1], [ac_cv_lib[]Name[]_libs], [
    AC_LIB_LINKFLAGS_BODY([$1], [$2])
    ac_cv_lib[]Name[]_libs="$LIB[]NAME"
    ac_cv_lib[]Name[]_ltlibs="$LTLIB[]NAME"
    ac_cv_lib[]Name[]_cppflags="$INC[]NAME"
  ])
  LIB[]NAME="$ac_cv_lib[]Name[]_libs"
  LTLIB[]NAME="$ac_cv_lib[]Name[]_ltlibs"
  INC[]NAME="$ac_cv_lib[]Name[]_cppflags"
  AC_LIB_APPENDTOVAR([CPPFLAGS], [$INC]NAME)
  AC_SUBST([LIB]NAME)
  AC_SUBST([LTLIB]NAME)
  dnl Also set HAVE_LIB[]NAME so that AC_LIB_HAVE_LINKFLAGS can reuse the
  dnl results of this search when this library appears as a dependency.
  HAVE_LIB[]NAME=yes
  undefine([Name])
  undefine([NAME])
])

dnl AC_LIB_HAVE_LINKFLAGS(name, dependencies, includes, testcode)
dnl searches for libname and the libraries corresponding to explicit and
dnl implicit dependencies, together with the specified include files and
dnl the ability to compile and link the specified testcode. If found, it
dnl sets and AC_SUBSTs HAVE_LIB${NAME}=yes and the LIB${NAME} and
dnl LTLIB${NAME} variables and augments the CPPFLAGS variable, and
dnl #defines HAVE_LIB${NAME} to 1. Otherwise, it sets and AC_SUBSTs
dnl HAVE_LIB${NAME}=no and LIB${NAME} and LTLIB${NAME} to empty.
AC_DEFUN([AC_LIB_HAVE_LINKFLAGS],
[
  AC_REQUIRE([AC_LIB_PREPARE_PREFIX])
  AC_REQUIRE([AC_LIB_RPATH])
  define([Name],[translit([$1],[./-], [___])])
  define([NAME],[translit([$1],[abcdefghijklmnopqrstuvwxyz./-],
                               [ABCDEFGHIJKLMNOPQRSTUVWXYZ___])])

  dnl Search for lib[]Name and define LIB[]NAME, LTLIB[]NAME and INC[]NAME
  dnl accordingly.
  AC_LIB_LINKFLAGS_BODY([$1], [$2])

  dnl Add $INC[]NAME to CPPFLAGS before performing the following checks,
  dnl because if the user has installed lib[]Name and not disabled its use
  dnl via --without-lib[]Name-prefix, he wants to use it.
  ac_save_CPPFLAGS="$CPPFLAGS"
  AC_LIB_APPENDTOVAR([CPPFLAGS], [$INC]NAME)

  AC_CACHE_CHECK([for lib[]$1], [ac_cv_lib[]Name], [
    ac_save_LIBS="$LIBS"
    LIBS="$LIBS $LIB[]NAME"
    AC_TRY_LINK([$3], [$4], [ac_cv_lib[]Name=yes], [ac_cv_lib[]Name=no])
    LIBS="$ac_save_LIBS"
  ])
  if test "$ac_cv_lib[]Name" = yes; then
    HAVE_LIB[]NAME=yes
    AC_DEFINE([HAVE_LIB]NAME, 1, [Define if you have the $1 library.])
    AC_MSG_CHECKING([how to link with lib[]$1])
    AC_MSG_RESULT([$LIB[]NAME])
  else
    HAVE_LIB[]NAME=no
    dnl If $LIB[]NAME didn't lead to a usable library, we don't need
    dnl $INC[]NAME either.
    CPPFLAGS="$ac_save_CPPFLAGS"
    LIB[]NAME=
    LTLIB[]NAME=
  fi
  AC_SUBST([HAVE_LIB]NAME)
  AC_SUBST([LIB]NAME)
  AC_SUBST([LTLIB]NAME)
  undefine([Name])
  undefine([NAME])
])

dnl Determine the platform dependent parameters needed to use rpath:
dnl libext, shlibext, hardcode_libdir_flag_spec, hardcode_libdir_separator,
dnl hardcode_direct, hardcode_minus_L,
dnl sys_lib_search_path_spec, sys_lib_dlsearch_path_spec.
AC_DEFUN([AC_LIB_RPATH],
[
  AC_REQUIRE([AC_PROG_CC])                dnl we use $CC, $GCC, $LDFLAGS
  AC_REQUIRE([AC_LIB_PROG_LD])            dnl we use $LD, $with_gnu_ld
  AC_REQUIRE([AC_CANONICAL_HOST])         dnl we use $host
  AC_REQUIRE([AC_CONFIG_AUX_DIR_DEFAULT]) dnl we use $ac_aux_dir
  AC_CACHE_CHECK([for shared library run path origin], acl_cv_rpath, [
    CC="$CC" GCC="$GCC" LDFLAGS="$LDFLAGS" LD="$LD" with_gnu_ld="$with_gnu_ld" \
    ${CONFIG_SHELL-/bin/sh} "$ac_aux_dir/config.rpath" "$host" > conftest.sh
    . ./conftest.sh
    rm -f ./conftest.sh
    acl_cv_rpath=done
  ])
  wl="$acl_cv_wl"
  libext="$acl_cv_libext"
  shlibext="$acl_cv_shlibext"
  hardcode_libdir_flag_spec="$acl_cv_hardcode_libdir_flag_spec"
  hardcode_libdir_separator="$acl_cv_hardcode_libdir_separator"
  hardcode_direct="$acl_cv_hardcode_direct"
  hardcode_minus_L="$acl_cv_hardcode_minus_L"
  sys_lib_search_path_spec="$acl_cv_sys_lib_search_path_spec"
  sys_lib_dlsearch_path_spec="$acl_cv_sys_lib_dlsearch_path_spec"
  dnl Determine whether the user wants rpath handling at all.
  AC_ARG_ENABLE(rpath,
    [  --disable-rpath         do not hardcode runtime library paths],
    :, enable_rpath=yes)
])

dnl AC_LIB_LINKFLAGS_BODY(name [, dependencies]) searches for libname and
dnl the libraries corresponding to explicit and implicit dependencies.
dnl Sets the LIB${NAME}, LTLIB${NAME} and INC${NAME} variables.
AC_DEFUN([AC_LIB_LINKFLAGS_BODY],
[
  define([NAME],[translit([$1],[abcdefghijklmnopqrstuvwxyz./-],
                               [ABCDEFGHIJKLMNOPQRSTUVWXYZ___])])
  dnl By default, look in $includedir and $libdir.
  use_additional=yes
  AC_LIB_WITH_FINAL_PREFIX([
    eval additional_includedir=\"$includedir\"
    eval additional_libdir=\"$libdir\"
  ])
  AC_ARG_WITH([lib$1-prefix],
[  --with-lib$1-prefix[=DIR]  search for lib$1 in DIR/include and DIR/lib
  --without-lib$1-prefix     don't search for lib$1 in includedir and libdir],
[
    if test "X$withval" = "Xno"; then
      use_additional=no
    else
      if test "X$withval" = "X"; then
        AC_LIB_WITH_FINAL_PREFIX([
          eval additional_includedir=\"$includedir\"
          eval additional_libdir=\"$libdir\"
        ])
      else
        additional_includedir="$withval/include"
        additional_libdir="$withval/lib"
      fi
    fi
])
  dnl Search the library and its dependencies in $additional_libdir and
  dnl $LDFLAGS. Using breadth-first-seach.
  LIB[]NAME=
  LTLIB[]NAME=
  INC[]NAME=
  rpathdirs=
  ltrpathdirs=
  names_already_handled=
  names_next_round='$1 $2'
  while test -n "$names_next_round"; do
    names_this_round="$names_next_round"
    names_next_round=
    for name in $names_this_round; do
      already_handled=
      for n in $names_already_handled; do
        if test "$n" = "$name"; then
          already_handled=yes
          break
        fi
      done
      if test -z "$already_handled"; then
        names_already_handled="$names_already_handled $name"
        dnl See if it was already located by an earlier AC_LIB_LINKFLAGS
        dnl or AC_LIB_HAVE_LINKFLAGS call.
        uppername=`echo "$name" | sed -e 'y|abcdefghijklmnopqrstuvwxyz./-|ABCDEFGHIJKLMNOPQRSTUVWXYZ___|'`
        eval value=\"\$HAVE_LIB$uppername\"
        if test -n "$value"; then
          if test "$value" = yes; then
            eval value=\"\$LIB$uppername\"
            test -z "$value" || LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$value"
            eval value=\"\$LTLIB$uppername\"
            test -z "$value" || LTLIB[]NAME="${LTLIB[]NAME}${LTLIB[]NAME:+ }$value"
          else
            dnl An earlier call to AC_LIB_HAVE_LINKFLAGS has determined
            dnl that this library doesn't exist. So just drop it.
            :
          fi
        else
          dnl Search the library lib$name in $additional_libdir and $LDFLAGS
          dnl and the already constructed $LIBNAME/$LTLIBNAME.
          found_dir=
          found_la=
          found_so=
          found_a=
          if test $use_additional = yes; then
            if test -n "$shlibext" && test -f "$additional_libdir/lib$name.$shlibext"; then
              found_dir="$additional_libdir"
              found_so="$additional_libdir/lib$name.$shlibext"
              if test -f "$additional_libdir/lib$name.la"; then
                found_la="$additional_libdir/lib$name.la"
              fi
            else
              if test -f "$additional_libdir/lib$name.$libext"; then
                found_dir="$additional_libdir"
                found_a="$additional_libdir/lib$name.$libext"
                if test -f "$additional_libdir/lib$name.la"; then
                  found_la="$additional_libdir/lib$name.la"
                fi
              fi
            fi
          fi
          if test "X$found_dir" = "X"; then
            for x in $LDFLAGS $LTLIB[]NAME; do
              AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
              case "$x" in
                -L*)
                  dir=`echo "X$x" | sed -e 's/^X-L//'`
                  if test -n "$shlibext" && test -f "$dir/lib$name.$shlibext"; then
                    found_dir="$dir"
                    found_so="$dir/lib$name.$shlibext"
                    if test -f "$dir/lib$name.la"; then
                      found_la="$dir/lib$name.la"
                    fi
                  else
                    if test -f "$dir/lib$name.$libext"; then
                      found_dir="$dir"
                      found_a="$dir/lib$name.$libext"
                      if test -f "$dir/lib$name.la"; then
                        found_la="$dir/lib$name.la"
                      fi
                    fi
                  fi
                  ;;
              esac
              if test "X$found_dir" != "X"; then
                break
              fi
            done
          fi
          if test "X$found_dir" != "X"; then
            dnl Found the library.
            LTLIB[]NAME="${LTLIB[]NAME}${LTLIB[]NAME:+ }-L$found_dir -l$name"
            if test "X$found_so" != "X"; then
              dnl Linking with a shared library. We attempt to hardcode its
              dnl directory into the executable's runpath, unless it's the
              dnl standard /usr/lib.
              if test "$enable_rpath" = no || test "X$found_dir" = "X/usr/lib"; then
                dnl No hardcoding is needed.
                LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$found_so"
              else
                dnl Use an explicit option to hardcode DIR into the resulting
                dnl binary.
                dnl Potentially add DIR to ltrpathdirs.
                dnl The ltrpathdirs will be appended to $LTLIBNAME at the end.
                haveit=
                for x in $ltrpathdirs; do
                  if test "X$x" = "X$found_dir"; then
                    haveit=yes
                    break
                  fi
                done
                if test -z "$haveit"; then
                  ltrpathdirs="$ltrpathdirs $found_dir"
                fi
                dnl The hardcoding into $LIBNAME is system dependent.
                if test "$hardcode_direct" = yes; then
                  dnl Using DIR/libNAME.so during linking hardcodes DIR into the
                  dnl resulting binary.
                  LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$found_so"
                else
                  if test -n "$hardcode_libdir_flag_spec" && test "$hardcode_minus_L" = no; then
                    dnl Use an explicit option to hardcode DIR into the resulting
                    dnl binary.
                    LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$found_so"
                    dnl Potentially add DIR to rpathdirs.
                    dnl The rpathdirs will be appended to $LIBNAME at the end.
                    haveit=
                    for x in $rpathdirs; do
                      if test "X$x" = "X$found_dir"; then
                        haveit=yes
                        break
                      fi
                    done
                    if test -z "$haveit"; then
                      rpathdirs="$rpathdirs $found_dir"
                    fi
                  else
                    dnl Rely on "-L$found_dir".
                    dnl But don't add it if it's already contained in the LDFLAGS
                    dnl or the already constructed $LIBNAME
                    haveit=
                    for x in $LDFLAGS $LIB[]NAME; do
                      AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
                      if test "X$x" = "X-L$found_dir"; then
                        haveit=yes
                        break
                      fi
                    done
                    if test -z "$haveit"; then
                      LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }-L$found_dir"
                    fi
                    if test "$hardcode_minus_L" != no; then
                      dnl FIXME: Not sure whether we should use
                      dnl "-L$found_dir -l$name" or "-L$found_dir $found_so"
                      dnl here.
                      LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$found_so"
                    else
                      dnl We cannot use $hardcode_runpath_var and LD_RUN_PATH
                      dnl here, because this doesn't fit in flags passed to the
                      dnl compiler. So give up. No hardcoding. This affects only
                      dnl very old systems.
                      dnl FIXME: Not sure whether we should use
                      dnl "-L$found_dir -l$name" or "-L$found_dir $found_so"
                      dnl here.
                      LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }-l$name"
                    fi
                  fi
                fi
              fi
            else
              if test "X$found_a" != "X"; then
                dnl Linking with a static library.
                LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$found_a"
              else
                dnl We shouldn't come here, but anyway it's good to have a
                dnl fallback.
                LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }-L$found_dir -l$name"
              fi
            fi
            dnl Assume the include files are nearby.
            additional_includedir=
            case "$found_dir" in
              */lib | */lib/)
                basedir=`echo "X$found_dir" | sed -e 's,^X,,' -e 's,/lib/*$,,'`
                additional_includedir="$basedir/include"
                ;;
            esac
            if test "X$additional_includedir" != "X"; then
              dnl Potentially add $additional_includedir to $INCNAME.
              dnl But don't add it
              dnl   1. if it's the standard /usr/include,
              dnl   2. if it's /usr/local/include and we are using GCC on Linux,
              dnl   3. if it's already present in $CPPFLAGS or the already
              dnl      constructed $INCNAME,
              dnl   4. if it doesn't exist as a directory.
              if test "X$additional_includedir" != "X/usr/include"; then
                haveit=
                if test "X$additional_includedir" = "X/usr/local/include"; then
                  if test -n "$GCC"; then
                    case $host_os in
                      linux*) haveit=yes;;
                    esac
                  fi
                fi
                if test -z "$haveit"; then
                  for x in $CPPFLAGS $INC[]NAME; do
                    AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
                    if test "X$x" = "X-I$additional_includedir"; then
                      haveit=yes
                      break
                    fi
                  done
                  if test -z "$haveit"; then
                    if test -d "$additional_includedir"; then
                      dnl Really add $additional_includedir to $INCNAME.
                      INC[]NAME="${INC[]NAME}${INC[]NAME:+ }-I$additional_includedir"
                    fi
                  fi
                fi
              fi
            fi
            dnl Look for dependencies.
            if test -n "$found_la"; then
              dnl Read the .la file. It defines the variables
              dnl dlname, library_names, old_library, dependency_libs, current,
              dnl age, revision, installed, dlopen, dlpreopen, libdir.
              save_libdir="$libdir"
              case "$found_la" in
                */* | *\\*) . "$found_la" ;;
                *) . "./$found_la" ;;
              esac
              libdir="$save_libdir"
              dnl We use only dependency_libs.
              for dep in $dependency_libs; do
                case "$dep" in
                  -L*)
                    additional_libdir=`echo "X$dep" | sed -e 's/^X-L//'`
                    dnl Potentially add $additional_libdir to $LIBNAME and $LTLIBNAME.
                    dnl But don't add it
                    dnl   1. if it's the standard /usr/lib,
                    dnl   2. if it's /usr/local/lib and we are using GCC on Linux,
                    dnl   3. if it's already present in $LDFLAGS or the already
                    dnl      constructed $LIBNAME,
                    dnl   4. if it doesn't exist as a directory.
                    if test "X$additional_libdir" != "X/usr/lib"; then
                      haveit=
                      if test "X$additional_libdir" = "X/usr/local/lib"; then
                        if test -n "$GCC"; then
                          case $host_os in
                            linux*) haveit=yes;;
                          esac
                        fi
                      fi
                      if test -z "$haveit"; then
                        haveit=
                        for x in $LDFLAGS $LIB[]NAME; do
                          AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
                          if test "X$x" = "X-L$additional_libdir"; then
                            haveit=yes
                            break
                          fi
                        done
                        if test -z "$haveit"; then
                          if test -d "$additional_libdir"; then
                            dnl Really add $additional_libdir to $LIBNAME.
                            LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }-L$additional_libdir"
                          fi
                        fi
                        haveit=
                        for x in $LDFLAGS $LTLIB[]NAME; do
                          AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
                          if test "X$x" = "X-L$additional_libdir"; then
                            haveit=yes
                            break
                          fi
                        done
                        if test -z "$haveit"; then
                          if test -d "$additional_libdir"; then
                            dnl Really add $additional_libdir to $LTLIBNAME.
                            LTLIB[]NAME="${LTLIB[]NAME}${LTLIB[]NAME:+ }-L$additional_libdir"
                          fi
                        fi
                      fi
                    fi
                    ;;
                  -R*)
                    dir=`echo "X$dep" | sed -e 's/^X-R//'`
                    if test "$enable_rpath" != no; then
                      dnl Potentially add DIR to rpathdirs.
                      dnl The rpathdirs will be appended to $LIBNAME at the end.
                      haveit=
                      for x in $rpathdirs; do
                        if test "X$x" = "X$dir"; then
                          haveit=yes
                          break
                        fi
                      done
                      if test -z "$haveit"; then
                        rpathdirs="$rpathdirs $dir"
                      fi
                      dnl Potentially add DIR to ltrpathdirs.
                      dnl The ltrpathdirs will be appended to $LTLIBNAME at the end.
                      haveit=
                      for x in $ltrpathdirs; do
                        if test "X$x" = "X$dir"; then
                          haveit=yes
                          break
                        fi
                      done
                      if test -z "$haveit"; then
                        ltrpathdirs="$ltrpathdirs $dir"
                      fi
                    fi
                    ;;
                  -l*)
                    dnl Handle this in the next round.
                    names_next_round="$names_next_round "`echo "X$dep" | sed -e 's/^X-l//'`
                    ;;
                  *.la)
                    dnl Handle this in the next round. Throw away the .la's
                    dnl directory; it is already contained in a preceding -L
                    dnl option.
                    names_next_round="$names_next_round "`echo "X$dep" | sed -e 's,^X.*/,,' -e 's,^lib,,' -e 's,\.la$,,'`
                    ;;
                  *)
                    dnl Most likely an immediate library name.
                    LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$dep"
                    LTLIB[]NAME="${LTLIB[]NAME}${LTLIB[]NAME:+ }$dep"
                    ;;
                esac
              done
            fi
          else
            dnl Didn't find the library; assume it is in the system directories
            dnl known to the linker and runtime loader. (All the system
            dnl directories known to the linker should also be known to the
            dnl runtime loader, otherwise the system is severely misconfigured.)
            LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }-l$name"
            LTLIB[]NAME="${LTLIB[]NAME}${LTLIB[]NAME:+ }-l$name"
          fi
        fi
      fi
    done
  done
  if test "X$rpathdirs" != "X"; then
    if test -n "$hardcode_libdir_separator"; then
      dnl Weird platform: only the last -rpath option counts, the user must
      dnl pass all path elements in one option. We can arrange that for a
      dnl single library, but not when more than one $LIBNAMEs are used.
      alldirs=
      for found_dir in $rpathdirs; do
        alldirs="${alldirs}${alldirs:+$hardcode_libdir_separator}$found_dir"
      done
      dnl Note: hardcode_libdir_flag_spec uses $libdir and $wl.
      acl_save_libdir="$libdir"
      libdir="$alldirs"
      eval flag=\"$hardcode_libdir_flag_spec\"
      libdir="$acl_save_libdir"
      LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$flag"
    else
      dnl The -rpath options are cumulative.
      for found_dir in $rpathdirs; do
        acl_save_libdir="$libdir"
        libdir="$found_dir"
        eval flag=\"$hardcode_libdir_flag_spec\"
        libdir="$acl_save_libdir"
        LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$flag"
      done
    fi
  fi
  if test "X$ltrpathdirs" != "X"; then
    dnl When using libtool, the option that works for both libraries and
    dnl executables is -R. The -R options are cumulative.
    for found_dir in $ltrpathdirs; do
      LTLIB[]NAME="${LTLIB[]NAME}${LTLIB[]NAME:+ }-R$found_dir"
    done
  fi
])

dnl AC_LIB_APPENDTOVAR(VAR, CONTENTS) appends the elements of CONTENTS to VAR,
dnl unless already present in VAR.
dnl Works only for CPPFLAGS, not for LIB* variables because that sometimes
dnl contains two or three consecutive elements that belong together.
AC_DEFUN([AC_LIB_APPENDTOVAR],
[
  for element in [$2]; do
    haveit=
    for x in $[$1]; do
      AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
      if test "X$x" = "X$element"; then
        haveit=yes
        break
      fi
    done
    if test -z "$haveit"; then
      [$1]="${[$1]}${[$1]:+ }$element"
    fi
  done
])

# lib-ld.m4 serial 1 (gettext-0.11)
dnl Copyright (C) 1996-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl Subroutines of libtool.m4,
dnl with replacements s/AC_/AC_LIB/ and s/lt_cv/acl_cv/ to avoid collision
dnl with libtool.m4.

dnl From libtool-1.4. Sets the variable with_gnu_ld to yes or no.
AC_DEFUN([AC_LIB_PROG_LD_GNU],
[AC_CACHE_CHECK([if the linker ($LD) is GNU ld], acl_cv_prog_gnu_ld,
[# I'd rather use --version here, but apparently some GNU ld's only accept -v.
if $LD -v 2>&1 </dev/null | egrep '(GNU|with BFD)' 1>&5; then
  acl_cv_prog_gnu_ld=yes
else
  acl_cv_prog_gnu_ld=no
fi])
with_gnu_ld=$acl_cv_prog_gnu_ld
])

dnl From libtool-1.4. Sets the variable LD.
AC_DEFUN([AC_LIB_PROG_LD],
[AC_ARG_WITH(gnu-ld,
[  --with-gnu-ld           assume the C compiler uses GNU ld [default=no]],
test "$withval" = no || with_gnu_ld=yes, with_gnu_ld=no)
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_CANONICAL_HOST])dnl
ac_prog=ld
if test "$GCC" = yes; then
  # Check if gcc -print-prog-name=ld gives a path.
  AC_MSG_CHECKING([for ld used by GCC])
  case $host in
  *-*-mingw*)
    # gcc leaves a trailing carriage return which upsets mingw
    ac_prog=`($CC -print-prog-name=ld) 2>&5 | tr -d '\015'` ;;
  *)
    ac_prog=`($CC -print-prog-name=ld) 2>&5` ;;
  esac
  case $ac_prog in
    # Accept absolute paths.
    [[\\/]* | [A-Za-z]:[\\/]*)]
      [re_direlt='/[^/][^/]*/\.\./']
      # Canonicalize the path of ld
      ac_prog=`echo $ac_prog| sed 's%\\\\%/%g'`
      while echo $ac_prog | grep "$re_direlt" > /dev/null 2>&1; do
	ac_prog=`echo $ac_prog| sed "s%$re_direlt%/%"`
      done
      test -z "$LD" && LD="$ac_prog"
      ;;
  "")
    # If it fails, then pretend we aren't using GCC.
    ac_prog=ld
    ;;
  *)
    # If it is relative, then search for the first ld in PATH.
    with_gnu_ld=unknown
    ;;
  esac
elif test "$with_gnu_ld" = yes; then
  AC_MSG_CHECKING([for GNU ld])
else
  AC_MSG_CHECKING([for non-GNU ld])
fi
AC_CACHE_VAL(acl_cv_path_LD,
[if test -z "$LD"; then
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}${PATH_SEPARATOR-:}"
  for ac_dir in $PATH; do
    test -z "$ac_dir" && ac_dir=.
    if test -f "$ac_dir/$ac_prog" || test -f "$ac_dir/$ac_prog$ac_exeext"; then
      acl_cv_path_LD="$ac_dir/$ac_prog"
      # Check to see if the program is GNU ld.  I'd rather use --version,
      # but apparently some GNU ld's only accept -v.
      # Break only if it was the GNU/non-GNU ld that we prefer.
      if "$acl_cv_path_LD" -v 2>&1 < /dev/null | egrep '(GNU|with BFD)' > /dev/null; then
	test "$with_gnu_ld" != no && break
      else
	test "$with_gnu_ld" != yes && break
      fi
    fi
  done
  IFS="$ac_save_ifs"
else
  acl_cv_path_LD="$LD" # Let the user override the test with a path.
fi])
LD="$acl_cv_path_LD"
if test -n "$LD"; then
  AC_MSG_RESULT($LD)
else
  AC_MSG_RESULT(no)
fi
test -z "$LD" && AC_MSG_ERROR([no acceptable ld found in \$PATH])
AC_LIB_PROG_LD_GNU
])

#serial 6

dnl From J. David Anglin.

dnl HPUX and other systems can't unlink shared text that is being executed.

AC_DEFUN([jm_FUNC_UNLINK_BUSY_TEXT],
[dnl
  AC_CACHE_CHECK([whether a running program can be unlinked],
    jm_cv_func_unlink_busy_text,
    [
      AC_TRY_RUN([
        main (argc, argv)
          int argc;
          char **argv;
        {
          if (!argc)
            exit (-1);
          exit (unlink (argv[0]));
        }
	],
      jm_cv_func_unlink_busy_text=yes,
      jm_cv_func_unlink_busy_text=no,
      jm_cv_func_unlink_busy_text=no
      )
    ]
  )

  if test $jm_cv_func_unlink_busy_text = no; then
    INSTALL=$ac_install_sh
  fi
])

#serial 12

dnl From Jim Meyering.
dnl
dnl This is not pretty.  I've just taken the autoconf code and wrapped
dnl it in an AC_DEFUN.
dnl

# jm_LIST_MOUNTED_FILESYSTEMS([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
AC_DEFUN([jm_LIST_MOUNTED_FILESYSTEMS],
  [
AC_CHECK_FUNCS(listmntent getmntinfo)
AC_CHECK_HEADERS(mntent.h sys/param.h sys/ucred.h sys/mount.h sys/fs_types.h)
    getfsstat_includes="\
$ac_includes_default
#if HAVE_SYS_PARAM_H
# include <sys/param.h> /* needed by powerpc-apple-darwin1.3.7 */
#endif
#if HAVE_SYS_UCRED_H
# include <sys/ucred.h> /* needed by powerpc-apple-darwin1.3.7 */
#endif
#if HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif
#if HAVE_SYS_FS_TYPES_H
# include <sys/fs_types.h> /* needed by powerpc-apple-darwin1.3.7 */
#endif
"
AC_CHECK_MEMBERS([struct fsstat.f_fstypename],,,[$getfsstat_includes])

# Determine how to get the list of mounted filesystems.
ac_list_mounted_fs=

# If the getmntent function is available but not in the standard library,
# make sure LIBS contains -lsun (on Irix4) or -lseq (on PTX).
AC_FUNC_GETMNTENT

# This test must precede the ones for getmntent because Unicos-9 is
# reported to have the getmntent function, but its support is incompatible
# with other getmntent implementations.

# NOTE: Normally, I wouldn't use a check for system type as I've done for
# `CRAY' below since that goes against the whole autoconf philosophy.  But
# I think there is too great a chance that some non-Cray system has a
# function named listmntent to risk the false positive.

if test -z "$ac_list_mounted_fs"; then
  # Cray UNICOS 9
  AC_MSG_CHECKING([for listmntent of Cray/Unicos-9])
  AC_CACHE_VAL(fu_cv_sys_mounted_cray_listmntent,
    [fu_cv_sys_mounted_cray_listmntent=no
      AC_EGREP_CPP(yes,
        [#ifdef _CRAY
yes
#endif
        ], [test $ac_cv_func_listmntent = yes \
	    && fu_cv_sys_mounted_cray_listmntent=yes]
      )
    ]
  )
  AC_MSG_RESULT($fu_cv_sys_mounted_cray_listmntent)
  if test $fu_cv_sys_mounted_cray_listmntent = yes; then
    ac_list_mounted_fs=found
    AC_DEFINE(MOUNTED_LISTMNTENT, 1,
      [Define if there is a function named listmntent that can be used to
   list all mounted filesystems. (UNICOS)])
  fi
fi

if test -z "$ac_list_mounted_fs"; then
  # AIX.
  AC_MSG_CHECKING([for mntctl function and struct vmount])
  AC_CACHE_VAL(fu_cv_sys_mounted_vmount,
  [AC_TRY_CPP([#include <fshelp.h>],
    fu_cv_sys_mounted_vmount=yes,
    fu_cv_sys_mounted_vmount=no)])
  AC_MSG_RESULT($fu_cv_sys_mounted_vmount)
  if test $fu_cv_sys_mounted_vmount = yes; then
    ac_list_mounted_fs=found
    AC_DEFINE(MOUNTED_VMOUNT, 1,
	[Define if there is a function named mntctl that can be used to read
   the list of mounted filesystems, and there is a system header file
   that declares `struct vmount.'  (AIX)])
  fi
fi

if test $ac_cv_func_getmntent = yes; then

  # This system has the getmntent function.
  # Determine whether it's the one-argument variant or the two-argument one.

  if test -z "$ac_list_mounted_fs"; then
    # 4.3BSD, SunOS, HP-UX, Dynix, Irix
    AC_MSG_CHECKING([for one-argument getmntent function])
    AC_CACHE_VAL(fu_cv_sys_mounted_getmntent1,
		 [AC_TRY_COMPILE([
/* SunOS 4.1.x /usr/include/mntent.h needs this for FILE */
#include <stdio.h>

#include <mntent.h>
#if !defined MOUNTED
# if defined _PATH_MOUNTED	/* GNU libc  */
#  define MOUNTED _PATH_MOUNTED
# endif
# if defined MNT_MNTTAB	/* HP-UX.  */
#  define MOUNTED MNT_MNTTAB
# endif
# if defined MNTTABNAME	/* Dynix.  */
#  define MOUNTED MNTTABNAME
# endif
#endif
],
                    [ struct mntent *mnt = 0; char *table = MOUNTED; ],
		    fu_cv_sys_mounted_getmntent1=yes,
		    fu_cv_sys_mounted_getmntent1=no)])
    AC_MSG_RESULT($fu_cv_sys_mounted_getmntent1)
    if test $fu_cv_sys_mounted_getmntent1 = yes; then
      ac_list_mounted_fs=found
      AC_DEFINE(MOUNTED_GETMNTENT1, 1,
  [Define if there is a function named getmntent for reading the list
   of mounted filesystems, and that function takes a single argument.
   (4.3BSD, SunOS, HP-UX, Dynix, Irix)])
    fi
  fi

  if test -z "$ac_list_mounted_fs"; then
    # SVR4
    AC_MSG_CHECKING([for two-argument getmntent function])
    AC_CACHE_VAL(fu_cv_sys_mounted_getmntent2,
    [AC_EGREP_HEADER(getmntent, sys/mnttab.h,
      fu_cv_sys_mounted_getmntent2=yes,
      fu_cv_sys_mounted_getmntent2=no)])
    AC_MSG_RESULT($fu_cv_sys_mounted_getmntent2)
    if test $fu_cv_sys_mounted_getmntent2 = yes; then
      ac_list_mounted_fs=found
      AC_DEFINE(MOUNTED_GETMNTENT2, 1,
  [Define if there is a function named getmntent for reading the list of
   mounted filesystems, and that function takes two arguments.  (SVR4)])
    fi
  fi

fi

if test -z "$ac_list_mounted_fs"; then
  # DEC Alpha running OSF/1, and Apple Darwin 1.3.
  # powerpc-apple-darwin1.3.7 needs sys/param.h sys/ucred.h sys/fs_types.h

  AC_MSG_CHECKING([for getfsstat function])
  AC_CACHE_VAL(fu_cv_sys_mounted_getfsstat,
  [AC_TRY_LINK([
#include <sys/types.h>
#if HAVE_STRUCT_FSSTAT_F_FSTYPENAME
# define FS_TYPE(Ent) ((Ent).f_fstypename)
#else
# define FS_TYPE(Ent) mnt_names[(Ent).f_type]
#endif
]$getfsstat_includes
,
  [struct statfs *stats;
   int numsys = getfsstat ((struct statfs *)0, 0L, MNT_WAIT);
   char *t = FS_TYPE (*stats); ],
    fu_cv_sys_mounted_getfsstat=yes,
    fu_cv_sys_mounted_getfsstat=no)])
  AC_MSG_RESULT($fu_cv_sys_mounted_getfsstat)
  if test $fu_cv_sys_mounted_getfsstat = yes; then
    ac_list_mounted_fs=found
    AC_DEFINE(MOUNTED_GETFSSTAT, 1,
	      [Define if there is a function named getfsstat for reading the
   list of mounted filesystems.  (DEC Alpha running OSF/1)])
  fi
fi

if test -z "$ac_list_mounted_fs"; then
  # SVR3
  AC_MSG_CHECKING([for FIXME existence of three headers])
  AC_CACHE_VAL(fu_cv_sys_mounted_fread_fstyp,
    [AC_TRY_CPP([
#include <sys/statfs.h>
#include <sys/fstyp.h>
#include <mnttab.h>],
		fu_cv_sys_mounted_fread_fstyp=yes,
		fu_cv_sys_mounted_fread_fstyp=no)])
  AC_MSG_RESULT($fu_cv_sys_mounted_fread_fstyp)
  if test $fu_cv_sys_mounted_fread_fstyp = yes; then
    ac_list_mounted_fs=found
    AC_DEFINE(MOUNTED_FREAD_FSTYP, 1,
[Define if (like SVR2) there is no specific function for reading the
   list of mounted filesystems, and your system has these header files:
   <sys/fstyp.h> and <sys/statfs.h>.  (SVR3)])
  fi
fi

if test -z "$ac_list_mounted_fs"; then
  # 4.4BSD and DEC OSF/1.
  AC_MSG_CHECKING([for getmntinfo function])
  AC_CACHE_VAL(fu_cv_sys_mounted_getmntinfo,
    [
      test "$ac_cv_func_getmntinfo" = yes \
	  && fu_cv_sys_mounted_getmntinfo=yes \
	  || fu_cv_sys_mounted_getmntinfo=no
    ])
  AC_MSG_RESULT($fu_cv_sys_mounted_getmntinfo)
  if test $fu_cv_sys_mounted_getmntinfo = yes; then
    ac_list_mounted_fs=found
    AC_DEFINE(MOUNTED_GETMNTINFO, 1,
	      [Define if there is a function named getmntinfo for reading the
   list of mounted filesystems.  (4.4BSD, Darwin)])
  fi
fi

if test -z "$ac_list_mounted_fs"; then
  # Ultrix
  AC_MSG_CHECKING([for getmnt function])
  AC_CACHE_VAL(fu_cv_sys_mounted_getmnt,
    [AC_TRY_CPP([
#include <sys/fs_types.h>
#include <sys/mount.h>],
		fu_cv_sys_mounted_getmnt=yes,
		fu_cv_sys_mounted_getmnt=no)])
  AC_MSG_RESULT($fu_cv_sys_mounted_getmnt)
  if test $fu_cv_sys_mounted_getmnt = yes; then
    ac_list_mounted_fs=found
    AC_DEFINE(MOUNTED_GETMNT, 1,
      [Define if there is a function named getmnt for reading the list of
   mounted filesystems.  (Ultrix)])
  fi
fi

if test -z "$ac_list_mounted_fs"; then
  # BeOS
  AC_CHECK_FUNCS(next_dev fs_stat_dev)
  AC_CHECK_HEADERS(fs_info.h)
  AC_MSG_CHECKING([for BEOS mounted file system support functions])
  if test $ac_cv_header_fs_info_h = yes \
      && test $ac_cv_func_next_dev = yes \
	&& test $ac_cv_func_fs_stat_dev = yes; then
    fu_result=yes
  else
    fu_result=no
  fi
  AC_MSG_RESULT($fu_result)
  if test $fu_result = yes; then
    ac_list_mounted_fs=found
    AC_DEFINE(MOUNTED_FS_STAT_DEV, 1,
      [Define if there are functions named next_dev and fs_stat_dev for
   reading the list of mounted filesystems.  (BeOS)])
  fi
fi

if test -z "$ac_list_mounted_fs"; then
  # SVR2
  AC_MSG_CHECKING([whether it is possible to resort to fread on /etc/mnttab])
  AC_CACHE_VAL(fu_cv_sys_mounted_fread,
    [AC_TRY_CPP([#include <mnttab.h>],
		fu_cv_sys_mounted_fread=yes,
		fu_cv_sys_mounted_fread=no)])
  AC_MSG_RESULT($fu_cv_sys_mounted_fread)
  if test $fu_cv_sys_mounted_fread = yes; then
    ac_list_mounted_fs=found
    AC_DEFINE(MOUNTED_FREAD, 1,
	      [Define if there is no specific function for reading the list of
   mounted filesystems.  fread will be used to read /etc/mnttab.  (SVR2) ])
  fi
fi

if test -z "$ac_list_mounted_fs"; then
  AC_MSG_ERROR([could not determine how to read list of mounted filesystems])
  # FIXME -- no need to abort building the whole package
  # Can't build mountlist.c or anything that needs its functions
fi

AS_IF([test $ac_list_mounted_fs = found], [$1], [$2])

  ])

#serial 3

dnl From Jim Meyering.
dnl
dnl See if struct statfs has the f_fstypename member.
dnl If so, define HAVE_F_FSTYPENAME_IN_STATFS.
dnl

AC_DEFUN([jm_FSTYPENAME],
  [
    AC_CACHE_CHECK([for f_fstypename in struct statfs],
		   fu_cv_sys_f_fstypename_in_statfs,
      [
	AC_TRY_COMPILE(
	  [
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
	  ],
	  [struct statfs s; int i = sizeof s.f_fstypename;],
	  fu_cv_sys_f_fstypename_in_statfs=yes,
	  fu_cv_sys_f_fstypename_in_statfs=no
	)
      ]
    )

    if test $fu_cv_sys_f_fstypename_in_statfs = yes; then
      AC_DEFINE(HAVE_F_FSTYPENAME_IN_STATFS, 1,
		[Define if struct statfs has the f_fstypename member.])
    fi
  ]
)

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

#serial 5

# Define some macros required for proper operation of code in lib/*.c
# on MSDOS/Windows systems.

# From Jim Meyering.

AC_DEFUN([jm_AC_DOS],
  [
    AC_CACHE_CHECK([whether system is Windows or MSDOS], [ac_cv_win_or_dos],
      [
        AC_TRY_COMPILE([],
        [#if !defined _WIN32 && !defined __WIN32__ && !defined __MSDOS__
neither MSDOS nor Windows
#endif],
        [ac_cv_win_or_dos=yes],
        [ac_cv_win_or_dos=no])
      ])

    if test x"$ac_cv_win_or_dos" = xyes; then
      ac_fs_accepts_drive_letter_prefix=1
      ac_fs_backslash_is_file_name_separator=1
    else
      ac_fs_accepts_drive_letter_prefix=0
      ac_fs_backslash_is_file_name_separator=0
    fi

    AH_VERBATIM(FILESYSTEM_PREFIX_LEN,
    [#if FILESYSTEM_ACCEPTS_DRIVE_LETTER_PREFIX
# define FILESYSTEM_PREFIX_LEN(Filename) \
  ((Filename)[0] && (Filename)[1] == ':' ? 2 : 0)
#else
# define FILESYSTEM_PREFIX_LEN(Filename) 0
#endif])

    AC_DEFINE_UNQUOTED([FILESYSTEM_ACCEPTS_DRIVE_LETTER_PREFIX],
      $ac_fs_accepts_drive_letter_prefix,
      [Define on systems for which file names may have a so-called
       `drive letter' prefix, define this to compute the length of that
       prefix, including the colon.])

    AH_VERBATIM(ISSLASH,
    [#if FILESYSTEM_BACKSLASH_IS_FILE_NAME_SEPARATOR
# define ISSLASH(C) ((C) == '/' || (C) == '\\')
#else
# define ISSLASH(C) ((C) == '/')
#endif])

    AC_DEFINE_UNQUOTED([FILESYSTEM_BACKSLASH_IS_FILE_NAME_SEPARATOR],
      $ac_fs_backslash_is_file_name_separator,
      [Define if the backslash character may also serve as a file name
       component separator.])
  ])

#serial 1
AC_DEFUN([AC_FUNC_CANONICALIZE_FILE_NAME],
  [
    AC_REQUIRE([AC_HEADER_STDC])
    AC_CHECK_HEADERS(string.h sys/param.h stddef.h)
    AC_CHECK_FUNCS(resolvepath)
    AC_REQUIRE([AC_HEADER_STAT])

    # This would simply be AC_REPLACE_FUNC([canonicalize_file_name])
    # if the function name weren't so long.  Besides, I would rather
    # not have underscores in file names.
    AC_CHECK_FUNC([canonicalize_file_name], , [AC_LIBOBJ(canonicalize)])
  ])

#serial 5

dnl From Paul Eggert.

# Define ST_MTIM_NSEC to be the nanoseconds member of struct stat's st_mtim,
# if it exists.

AC_DEFUN([AC_STRUCT_ST_MTIM_NSEC],
 [AC_CACHE_CHECK([for nanoseconds member of struct stat.st_mtim],
   ac_cv_struct_st_mtim_nsec,
   [ac_save_CPPFLAGS="$CPPFLAGS"
    ac_cv_struct_st_mtim_nsec=no
    # tv_nsec -- the usual case
    # _tv_nsec -- Solaris 2.6, if
    #	(defined _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED == 1
    #	 && !defined __EXTENSIONS__)
    # st__tim.tv_nsec -- UnixWare 2.1.2
    for ac_val in tv_nsec _tv_nsec st__tim.tv_nsec; do
      CPPFLAGS="$ac_save_CPPFLAGS -DST_MTIM_NSEC=$ac_val"
      AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>], [struct stat s; s.st_mtim.ST_MTIM_NSEC;],
        [ac_cv_struct_st_mtim_nsec=$ac_val; break])
    done
    CPPFLAGS="$ac_save_CPPFLAGS"])

  if test $ac_cv_struct_st_mtim_nsec != no; then
    AC_DEFINE_UNQUOTED(ST_MTIM_NSEC, $ac_cv_struct_st_mtim_nsec,
      [Define to be the nanoseconds member of struct stat's st_mtim,
   if it exists.])
  fi
 ]
)

#serial 3

# Define HAVE_ST_DM_MODE if struct stat has an st_dm_mode member.

AC_DEFUN([AC_STRUCT_ST_DM_MODE],
 [AC_CACHE_CHECK([for st_dm_mode in struct stat], ac_cv_struct_st_dm_mode,
   [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>], [struct stat s; s.st_dm_mode;],
     ac_cv_struct_st_dm_mode=yes,
     ac_cv_struct_st_dm_mode=no)])

  if test $ac_cv_struct_st_dm_mode = yes; then
    AC_DEFINE(HAVE_ST_DM_MODE, 1,
	      [Define if struct stat has an st_dm_mode member. ])
  fi
 ]
)

#serial 5

dnl From Jim Meyering

dnl Define HAVE_STRUCT_TIMESPEC if `struct timespec' is declared
dnl in time.h or sys/time.h.

AC_DEFUN([jm_CHECK_TYPE_STRUCT_TIMESPEC],
[
  AC_REQUIRE([AC_HEADER_TIME])
  AC_CACHE_CHECK([for struct timespec], fu_cv_sys_struct_timespec,
    [AC_TRY_COMPILE(
      [
#      if TIME_WITH_SYS_TIME
#       include <sys/time.h>
#       include <time.h>
#      else
#       if HAVE_SYS_TIME_H
#        include <sys/time.h>
#       else
#        include <time.h>
#       endif
#      endif
      ],
      [static struct timespec x; x.tv_sec = x.tv_nsec;],
      fu_cv_sys_struct_timespec=yes,
      fu_cv_sys_struct_timespec=no)
    ])

  if test $fu_cv_sys_struct_timespec = yes; then
    AC_DEFINE(HAVE_STRUCT_TIMESPEC, 1,
	      [Define if struct timespec is declared in <time.h>. ])
  fi
])

#serial 6
dnl From Jim Meyering and Paul Eggert.
AC_DEFUN([jm_HEADER_TIOCGWINSZ_IN_TERMIOS_H],
[AC_REQUIRE([AC_SYS_POSIX_TERMIOS])
 AC_CACHE_CHECK([whether use of TIOCGWINSZ requires termios.h],
	        jm_cv_sys_tiocgwinsz_needs_termios_h,
  [jm_cv_sys_tiocgwinsz_needs_termios_h=no

   if test $ac_cv_sys_posix_termios = yes; then
     AC_EGREP_CPP([yes],
     [#include <sys/types.h>
#      include <termios.h>
#      ifdef TIOCGWINSZ
         yes
#      endif
     ], jm_cv_sys_tiocgwinsz_needs_termios_h=yes)
   fi
  ])
])

AC_DEFUN([jm_WINSIZE_IN_PTEM],
  [AC_REQUIRE([AC_SYS_POSIX_TERMIOS])
   AC_CACHE_CHECK([whether use of struct winsize requires sys/ptem.h],
     jm_cv_sys_struct_winsize_needs_sys_ptem_h,
     [jm_cv_sys_struct_winsize_needs_sys_ptem_h=yes
      if test $ac_cv_sys_posix_termios = yes; then
	AC_TRY_COMPILE([#include <termios.h>]
	  [struct winsize x;],
          [jm_cv_sys_struct_winsize_needs_sys_ptem_h=no])
      fi
      if test $jm_cv_sys_struct_winsize_needs_sys_ptem_h = yes; then
	AC_TRY_COMPILE([#include <sys/ptem.h>],
	  [struct winsize x;],
	  [], [jm_cv_sys_struct_winsize_needs_sys_ptem_h=no])
      fi])
   if test $jm_cv_sys_struct_winsize_needs_sys_ptem_h = yes; then
     AC_DEFINE([WINSIZE_IN_PTEM], 1,
       [Define if sys/ptem.h is required for struct winsize.])
   fi])

# Determine whether this system has infrastructure for obtaining the boot time.

# GNULIB_BOOT_TIME([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
* ----------------------------------------------------------
AC_DEFUN([GNULIB_BOOT_TIME],
[
  AC_CHECK_FUNCS(sysctl)
  AC_CHECK_HEADERS(sys/sysctl.h)
  AC_CACHE_CHECK(
    [whether we can get the system boot time],
    [gnulib_cv_have_boot_time],
    [
      AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
[AC_INCLUDES_DEFAULT
#if HAVE_SYSCTL && HAVE_SYS_SYSCTL_H
# include <sys/param.h> /* needed for OpenBSD 3.0 */
# include <sys/sysctl.h>
#endif
#ifdef HAVE_UTMPX_H
# include <utmpx.h>
#else
# include <utmp.h>
#endif
],
[[
#if defined BOOT_TIME || (defined CTL_KERN && defined KERN_BOOTTIME)
/* your system *does* have the infrastructure to determine boot time */
#else
please_tell_us_how_to_determine_boot_time_on_your_system
#endif
]])],
       gnulib_cv_have_boot_time=yes,
       gnulib_cv_have_boot_time=no)
    ])
  AS_IF([test $gnulib_cv_have_boot_time = yes], [$1], [$2])
])

#serial 4

AC_DEFUN([jm_HEADER_TIOCGWINSZ_NEEDS_SYS_IOCTL],
[AC_REQUIRE([jm_HEADER_TIOCGWINSZ_IN_TERMIOS_H])
 AC_CACHE_CHECK([whether use of TIOCGWINSZ requires sys/ioctl.h],
	        jm_cv_sys_tiocgwinsz_needs_sys_ioctl_h,
  [jm_cv_sys_tiocgwinsz_needs_sys_ioctl_h=no

  if test $jm_cv_sys_tiocgwinsz_needs_termios_h = no; then
    AC_EGREP_CPP([yes],
    [#include <sys/types.h>
#     include <sys/ioctl.h>
#     ifdef TIOCGWINSZ
        yes
#     endif
    ], jm_cv_sys_tiocgwinsz_needs_sys_ioctl_h=yes)
  fi
  ])
  if test $jm_cv_sys_tiocgwinsz_needs_sys_ioctl_h = yes; then
    AC_DEFINE(GWINSZ_IN_SYS_IOCTL, 1,
      [Define if your system defines TIOCGWINSZ in sys/ioctl.h.])
  fi
])

#serial 5

dnl Misc lib-related macros for fileutils, sh-utils, textutils.

AC_DEFUN([jm_LIB_CHECK],
[

  # Check for libypsec.a on Dolphin M88K machines.
  AC_CHECK_LIB(ypsec, main)

  # m88k running dgux 5.4 needs this
  AC_CHECK_LIB(ldgc, main)

  # Some programs need to link with -lm.  printf does if it uses
  # lib/strtod.c which uses pow.  And seq uses the math functions,
  # floor, modf, rint.  And factor uses sqrt.  And sleep uses fesetround.

  # Save a copy of $LIBS and add $FLOOR_LIBM before these tests
  # Check for these math functions used by seq.
  ac_su_saved_lib="$LIBS"
  LIBS="$LIBS -lm"
  AC_CHECK_FUNCS(floor modf rint)
  LIBS="$ac_su_saved_lib"

  AC_SUBST(SQRT_LIBM)
  AC_CHECK_FUNCS(sqrt)
  if test $ac_cv_func_sqrt = no; then
    AC_CHECK_LIB(m, sqrt, [SQRT_LIBM=-lm])
  fi

  AC_SUBST(FESETROUND_LIBM)
  AC_CHECK_FUNCS(fesetround)
  if test $ac_cv_func_fesetround = no; then
    AC_CHECK_LIB(m, fesetround, [FESETROUND_LIBM=-lm])
  fi

  # The -lsun library is required for YP support on Irix-4.0.5 systems.
  # m88k/svr3 DolphinOS systems using YP need -lypsec for id.
  AC_SEARCH_LIBS(yp_match, [sun ypsec])

  # SysV needs -lsec, older versions of Linux need -lshadow for
  # shadow passwords.  UnixWare 7 needs -lgen.
  AC_SEARCH_LIBS(getspnam, [shadow sec gen])

  AC_CHECK_HEADERS(shadow.h)

  # Requirements for su.c.
  shadow_includes="\
$ac_includes_default
#if HAVE_SHADOW_H
# include <shadow.h>
#endif
"
  AC_CHECK_MEMBERS([struct spwd.sp_pwdp],,,[$shadow_includes])
  AC_CHECK_FUNCS(getspnam)

  # SCO-ODT-3.0 is reported to need -lufc for crypt.
  # NetBSD needs -lcrypt for crypt.
  ac_su_saved_lib="$LIBS"
  AC_SEARCH_LIBS(crypt, [ufc crypt], [LIB_CRYPT="$ac_cv_search_crypt"])
  LIBS="$ac_su_saved_lib"
  AC_SUBST(LIB_CRYPT)
])

# gettext.m4 serial 17 (gettext-0.11.5)
dnl Copyright (C) 1995-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.
dnl
dnl This file can can be used in projects which are not available under
dnl the GNU General Public License or the GNU Library General Public
dnl License but which still want to provide support for the GNU gettext
dnl functionality.
dnl Please note that the actual code of the GNU gettext library is covered
dnl by the GNU Library General Public License, and the rest of the GNU
dnl gettext package package is covered by the GNU General Public License.
dnl They are *not* in the public domain.

dnl Authors:
dnl   Ulrich Drepper <drepper@cygnus.com>, 1995-2000.
dnl   Bruno Haible <haible@clisp.cons.org>, 2000-2002.

dnl Macro to add for using GNU gettext.

dnl Usage: AM_GNU_GETTEXT([INTLSYMBOL], [NEEDSYMBOL], [INTLDIR]).
dnl INTLSYMBOL can be one of 'external', 'no-libtool', 'use-libtool'. The
dnl    default (if it is not specified or empty) is 'no-libtool'.
dnl    INTLSYMBOL should be 'external' for packages with no intl directory,
dnl    and 'no-libtool' or 'use-libtool' for packages with an intl directory.
dnl    If INTLSYMBOL is 'use-libtool', then a libtool library
dnl    $(top_builddir)/intl/libintl.la will be created (shared and/or static,
dnl    depending on --{enable,disable}-{shared,static} and on the presence of
dnl    AM-DISABLE-SHARED). If INTLSYMBOL is 'no-libtool', a static library
dnl    $(top_builddir)/intl/libintl.a will be created.
dnl If NEEDSYMBOL is specified and is 'need-ngettext', then GNU gettext
dnl    implementations (in libc or libintl) without the ngettext() function
dnl    will be ignored.  If NEEDSYMBOL is specified and is
dnl    'need-formatstring-macros', then GNU gettext implementations that don't
dnl    support the ISO C 99 <inttypes.h> formatstring macros will be ignored.
dnl INTLDIR is used to find the intl libraries.  If empty,
dnl    the value `$(top_builddir)/intl/' is used.
dnl
dnl The result of the configuration is one of three cases:
dnl 1) GNU gettext, as included in the intl subdirectory, will be compiled
dnl    and used.
dnl    Catalog format: GNU --> install in $(datadir)
dnl    Catalog extension: .mo after installation, .gmo in source tree
dnl 2) GNU gettext has been found in the system's C library.
dnl    Catalog format: GNU --> install in $(datadir)
dnl    Catalog extension: .mo after installation, .gmo in source tree
dnl 3) No internationalization, always use English msgid.
dnl    Catalog format: none
dnl    Catalog extension: none
dnl If INTLSYMBOL is 'external', only cases 2 and 3 can occur.
dnl The use of .gmo is historical (it was needed to avoid overwriting the
dnl GNU format catalogs when building on a platform with an X/Open gettext),
dnl but we keep it in order not to force irrelevant filename changes on the
dnl maintainers.
dnl
AC_DEFUN([AM_GNU_GETTEXT],
[
  dnl Argument checking.
  ifelse([$1], [], , [ifelse([$1], [external], , [ifelse([$1], [no-libtool], , [ifelse([$1], [use-libtool], ,
    [errprint([ERROR: invalid first argument to AM_GNU_GETTEXT
])])])])])
  ifelse([$2], [], , [ifelse([$2], [need-ngettext], , [ifelse([$2], [need-formatstring-macros], ,
    [errprint([ERROR: invalid second argument to AM_GNU_GETTEXT
])])])])
  define(gt_included_intl, ifelse([$1], [external], [no], [yes]))
  define(gt_libtool_suffix_prefix, ifelse([$1], [use-libtool], [l], []))

  AC_REQUIRE([AM_PO_SUBDIRS])dnl
  ifelse(gt_included_intl, yes, [
    AC_REQUIRE([AM_INTL_SUBDIR])dnl
  ])

  dnl Prerequisites of AC_LIB_LINKFLAGS_BODY.
  AC_REQUIRE([AC_LIB_PREPARE_PREFIX])
  AC_REQUIRE([AC_LIB_RPATH])

  dnl Sometimes libintl requires libiconv, so first search for libiconv.
  dnl Ideally we would do this search only after the
  dnl      if test "$USE_NLS" = "yes"; then
  dnl        if test "$gt_cv_func_gnugettext_libc" != "yes"; then
  dnl tests. But if configure.in invokes AM_ICONV after AM_GNU_GETTEXT
  dnl the configure script would need to contain the same shell code
  dnl again, outside any 'if'. There are two solutions:
  dnl - Invoke AM_ICONV_LINKFLAGS_BODY here, outside any 'if'.
  dnl - Control the expansions in more detail using AC_PROVIDE_IFELSE.
  dnl Since AC_PROVIDE_IFELSE is only in autoconf >= 2.52 and not
  dnl documented, we avoid it.
  ifelse(gt_included_intl, yes, , [
    AC_REQUIRE([AM_ICONV_LINKFLAGS_BODY])
  ])

  AC_MSG_CHECKING([whether NLS is requested])
  dnl Default is enabled NLS
  AC_ARG_ENABLE(nls,
    [  --disable-nls           do not use Native Language Support],
    USE_NLS=$enableval, USE_NLS=yes)
  AC_MSG_RESULT($USE_NLS)
  AC_SUBST(USE_NLS)

  ifelse(gt_included_intl, yes, [
    BUILD_INCLUDED_LIBINTL=no
    USE_INCLUDED_LIBINTL=no
  ])
  LIBINTL=
  LTLIBINTL=
  POSUB=

  dnl If we use NLS figure out what method
  if test "$USE_NLS" = "yes"; then
    gt_use_preinstalled_gnugettext=no
    ifelse(gt_included_intl, yes, [
      AC_MSG_CHECKING([whether included gettext is requested])
      AC_ARG_WITH(included-gettext,
        [  --with-included-gettext use the GNU gettext library included here],
        nls_cv_force_use_gnu_gettext=$withval,
        nls_cv_force_use_gnu_gettext=no)
      AC_MSG_RESULT($nls_cv_force_use_gnu_gettext)

      nls_cv_use_gnu_gettext="$nls_cv_force_use_gnu_gettext"
      if test "$nls_cv_force_use_gnu_gettext" != "yes"; then
    ])
        dnl User does not insist on using GNU NLS library.  Figure out what
        dnl to use.  If GNU gettext is available we use this.  Else we have
        dnl to fall back to GNU NLS library.

        dnl Add a version number to the cache macros.
        define([gt_api_version], ifelse([$2], [need-formatstring-macros], 3, ifelse([$2], [need-ngettext], 2, 1)))
        define([gt_cv_func_gnugettext_libc], [gt_cv_func_gnugettext]gt_api_version[_libc])
        define([gt_cv_func_gnugettext_libintl], [gt_cv_func_gnugettext]gt_api_version[_libintl])

        AC_CACHE_CHECK([for GNU gettext in libc], gt_cv_func_gnugettext_libc,
         [AC_TRY_LINK([#include <libintl.h>
]ifelse([$2], [need-formatstring-macros],
[#ifndef __GNU_GETTEXT_SUPPORTED_REVISION
#define __GNU_GETTEXT_SUPPORTED_REVISION(major) ((major) == 0 ? 0 : -1)
#endif
changequote(,)dnl
typedef int array [2 * (__GNU_GETTEXT_SUPPORTED_REVISION(0) >= 1) - 1];
changequote([,])dnl
], [])[extern int _nl_msg_cat_cntr;
extern int *_nl_domain_bindings;],
            [bindtextdomain ("", "");
return (int) gettext ("")]ifelse([$2], [need-ngettext], [ + (int) ngettext ("", "", 0)], [])[ + _nl_msg_cat_cntr + *_nl_domain_bindings],
            gt_cv_func_gnugettext_libc=yes,
            gt_cv_func_gnugettext_libc=no)])

        if test "$gt_cv_func_gnugettext_libc" != "yes"; then
          dnl Sometimes libintl requires libiconv, so first search for libiconv.
          ifelse(gt_included_intl, yes, , [
            AM_ICONV_LINK
          ])
          dnl Search for libintl and define LIBINTL, LTLIBINTL and INCINTL
          dnl accordingly. Don't use AC_LIB_LINKFLAGS_BODY([intl],[iconv])
          dnl because that would add "-liconv" to LIBINTL and LTLIBINTL
          dnl even if libiconv doesn't exist.
          AC_LIB_LINKFLAGS_BODY([intl])
          AC_CACHE_CHECK([for GNU gettext in libintl],
            gt_cv_func_gnugettext_libintl,
           [gt_save_CPPFLAGS="$CPPFLAGS"
            CPPFLAGS="$CPPFLAGS $INCINTL"
            gt_save_LIBS="$LIBS"
            LIBS="$LIBS $LIBINTL"
            dnl Now see whether libintl exists and does not depend on libiconv.
            AC_TRY_LINK([#include <libintl.h>
]ifelse([$2], [need-formatstring-macros],
[#ifndef __GNU_GETTEXT_SUPPORTED_REVISION
#define __GNU_GETTEXT_SUPPORTED_REVISION(major) ((major) == 0 ? 0 : -1)
#endif
changequote(,)dnl
typedef int array [2 * (__GNU_GETTEXT_SUPPORTED_REVISION(0) >= 1) - 1];
changequote([,])dnl
], [])[extern int _nl_msg_cat_cntr;
extern
#ifdef __cplusplus
"C"
#endif
const char *_nl_expand_alias ();],
              [bindtextdomain ("", "");
return (int) gettext ("")]ifelse([$2], [need-ngettext], [ + (int) ngettext ("", "", 0)], [])[ + _nl_msg_cat_cntr + *_nl_expand_alias (0)],
              gt_cv_func_gnugettext_libintl=yes,
              gt_cv_func_gnugettext_libintl=no)
            dnl Now see whether libintl exists and depends on libiconv.
            if test "$gt_cv_func_gnugettext_libintl" != yes && test -n "$LIBICONV"; then
              LIBS="$LIBS $LIBICONV"
              AC_TRY_LINK([#include <libintl.h>
]ifelse([$2], [need-formatstring-macros],
[#ifndef __GNU_GETTEXT_SUPPORTED_REVISION
#define __GNU_GETTEXT_SUPPORTED_REVISION(major) ((major) == 0 ? 0 : -1)
#endif
changequote(,)dnl
typedef int array [2 * (__GNU_GETTEXT_SUPPORTED_REVISION(0) >= 1) - 1];
changequote([,])dnl
], [])[extern int _nl_msg_cat_cntr;
extern
#ifdef __cplusplus
"C"
#endif
const char *_nl_expand_alias ();],
                [bindtextdomain ("", "");
return (int) gettext ("")]ifelse([$2], [need-ngettext], [ + (int) ngettext ("", "", 0)], [])[ + _nl_msg_cat_cntr + *_nl_expand_alias (0)],
               [LIBINTL="$LIBINTL $LIBICONV"
                LTLIBINTL="$LTLIBINTL $LTLIBICONV"
                gt_cv_func_gnugettext_libintl=yes
               ])
            fi
            CPPFLAGS="$gt_save_CPPFLAGS"
            LIBS="$gt_save_LIBS"])
        fi

        dnl If an already present or preinstalled GNU gettext() is found,
        dnl use it.  But if this macro is used in GNU gettext, and GNU
        dnl gettext is already preinstalled in libintl, we update this
        dnl libintl.  (Cf. the install rule in intl/Makefile.in.)
        if test "$gt_cv_func_gnugettext_libc" = "yes" \
           || { test "$gt_cv_func_gnugettext_libintl" = "yes" \
                && test "$PACKAGE" != gettext; }; then
          gt_use_preinstalled_gnugettext=yes
        else
          dnl Reset the values set by searching for libintl.
          LIBINTL=
          LTLIBINTL=
          INCINTL=
        fi

    ifelse(gt_included_intl, yes, [
        if test "$gt_use_preinstalled_gnugettext" != "yes"; then
          dnl GNU gettext is not found in the C library.
          dnl Fall back on included GNU gettext library.
          nls_cv_use_gnu_gettext=yes
        fi
      fi

      if test "$nls_cv_use_gnu_gettext" = "yes"; then
        dnl Mark actions used to generate GNU NLS library.
        INTLOBJS="\$(GETTOBJS)"
        BUILD_INCLUDED_LIBINTL=yes
        USE_INCLUDED_LIBINTL=yes
        LIBINTL="ifelse([$3],[],\${top_builddir}/intl,[$3])/libintl.[]gt_libtool_suffix_prefix[]a $LIBICONV"
        LTLIBINTL="ifelse([$3],[],\${top_builddir}/intl,[$3])/libintl.[]gt_libtool_suffix_prefix[]a $LTLIBICONV"
        LIBS=`echo " $LIBS " | sed -e 's/ -lintl / /' -e 's/^ //' -e 's/ $//'`
      fi

      if test "$gt_use_preinstalled_gnugettext" = "yes" \
         || test "$nls_cv_use_gnu_gettext" = "yes"; then
        dnl Mark actions to use GNU gettext tools.
        CATOBJEXT=.gmo
      fi
    ])

    if test "$gt_use_preinstalled_gnugettext" = "yes" \
       || test "$nls_cv_use_gnu_gettext" = "yes"; then
      AC_DEFINE(ENABLE_NLS, 1,
        [Define to 1 if translation of program messages to the user's native language
   is requested.])
    else
      USE_NLS=no
    fi
  fi

  if test "$USE_NLS" = "yes"; then

    if test "$gt_use_preinstalled_gnugettext" = "yes"; then
      if test "$gt_cv_func_gnugettext_libintl" = "yes"; then
        AC_MSG_CHECKING([how to link with libintl])
        AC_MSG_RESULT([$LIBINTL])
        AC_LIB_APPENDTOVAR([CPPFLAGS], [$INCINTL])
      fi

      dnl For backward compatibility. Some packages may be using this.
      AC_DEFINE(HAVE_GETTEXT, 1,
       [Define if the GNU gettext() function is already present or preinstalled.])
      AC_DEFINE(HAVE_DCGETTEXT, 1,
       [Define if the GNU dcgettext() function is already present or preinstalled.])
    fi

    dnl We need to process the po/ directory.
    POSUB=po
  fi

  ifelse(gt_included_intl, yes, [
    dnl If this is used in GNU gettext we have to set BUILD_INCLUDED_LIBINTL
    dnl to 'yes' because some of the testsuite requires it.
    if test "$PACKAGE" = gettext; then
      BUILD_INCLUDED_LIBINTL=yes
    fi

    dnl Make all variables we use known to autoconf.
    AC_SUBST(BUILD_INCLUDED_LIBINTL)
    AC_SUBST(USE_INCLUDED_LIBINTL)
    AC_SUBST(CATOBJEXT)
    AC_SUBST(INTLOBJS)

    dnl For backward compatibility. Some configure.ins may be using this.
    nls_cv_header_intl=
    nls_cv_header_libgt=

    dnl For backward compatibility. Some Makefiles may be using this.
    DATADIRNAME=share
    AC_SUBST(DATADIRNAME)

    dnl For backward compatibility. Some Makefiles may be using this.
    INSTOBJEXT=.mo
    AC_SUBST(INSTOBJEXT)

    dnl For backward compatibility. Some Makefiles may be using this.
    GENCAT=gencat
    AC_SUBST(GENCAT)

    dnl Enable libtool support if the surrounding package wishes it.
    INTL_LIBTOOL_SUFFIX_PREFIX=gt_libtool_suffix_prefix
    AC_SUBST(INTL_LIBTOOL_SUFFIX_PREFIX)
  ])

  dnl For backward compatibility. Some Makefiles may be using this.
  INTLLIBS="$LIBINTL"
  AC_SUBST(INTLLIBS)

  dnl Make all documented variables known to autoconf.
  AC_SUBST(LIBINTL)
  AC_SUBST(LTLIBINTL)
  AC_SUBST(POSUB)
])


dnl Checks for all prerequisites of the po subdirectory,
dnl except for USE_NLS.
AC_DEFUN([AM_PO_SUBDIRS],
[
  AC_REQUIRE([AC_PROG_MAKE_SET])dnl
  AC_REQUIRE([AC_PROG_INSTALL])dnl
  AC_REQUIRE([AM_MKINSTALLDIRS])dnl

  dnl Perform the following tests also if --disable-nls has been given,
  dnl because they are needed for "make dist" to work.

  dnl Search for GNU msgfmt in the PATH.
  dnl The first test excludes Solaris msgfmt and early GNU msgfmt versions.
  dnl The second test excludes FreeBSD msgfmt.
  AM_PATH_PROG_WITH_TEST(MSGFMT, msgfmt,
    [$ac_dir/$ac_word --statistics /dev/null >/dev/null 2>&1 &&
     (if $ac_dir/$ac_word --statistics /dev/null 2>&1 >/dev/null | grep usage >/dev/null; then exit 1; else exit 0; fi)],
    :)
  AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)

  dnl Search for GNU xgettext 0.11 or newer in the PATH.
  dnl The first test excludes Solaris xgettext and early GNU xgettext versions.
  dnl The second test excludes FreeBSD xgettext.
  AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
    [$ac_dir/$ac_word --omit-header --copyright-holder= /dev/null >/dev/null 2>&1 &&
     (if $ac_dir/$ac_word --omit-header --copyright-holder= /dev/null 2>&1 >/dev/null | grep usage >/dev/null; then exit 1; else exit 0; fi)],
    :)
  dnl Remove leftover from FreeBSD xgettext call.
  rm -f messages.po

  dnl Search for GNU msgmerge 0.11 or newer in the PATH.
  AM_PATH_PROG_WITH_TEST(MSGMERGE, msgmerge,
    [$ac_dir/$ac_word --update -q /dev/null /dev/null >/dev/null 2>&1], :)

  dnl This could go away some day; the PATH_PROG_WITH_TEST already does it.
  dnl Test whether we really found GNU msgfmt.
  if test "$GMSGFMT" != ":"; then
    dnl If it is no GNU msgfmt we define it as : so that the
    dnl Makefiles still can work.
    if $GMSGFMT --statistics /dev/null >/dev/null 2>&1 &&
       (if $GMSGFMT --statistics /dev/null 2>&1 >/dev/null | grep usage >/dev/null; then exit 1; else exit 0; fi); then
      : ;
    else
      GMSGFMT=`echo "$GMSGFMT" | sed -e 's,^.*/,,'`
      AC_MSG_RESULT(
        [found $GMSGFMT program is not GNU msgfmt; ignore it])
      GMSGFMT=":"
    fi
  fi

  dnl This could go away some day; the PATH_PROG_WITH_TEST already does it.
  dnl Test whether we really found GNU xgettext.
  if test "$XGETTEXT" != ":"; then
    dnl If it is no GNU xgettext we define it as : so that the
    dnl Makefiles still can work.
    if $XGETTEXT --omit-header --copyright-holder= /dev/null >/dev/null 2>&1 &&
       (if $XGETTEXT --omit-header --copyright-holder= /dev/null 2>&1 >/dev/null | grep usage >/dev/null; then exit 1; else exit 0; fi); then
      : ;
    else
      AC_MSG_RESULT(
        [found xgettext program is not GNU xgettext; ignore it])
      XGETTEXT=":"
    fi
    dnl Remove leftover from FreeBSD xgettext call.
    rm -f messages.po
  fi

  AC_OUTPUT_COMMANDS([
    for ac_file in $CONFIG_FILES; do
      # Support "outfile[:infile[:infile...]]"
      case "$ac_file" in
        *:*) ac_file=`echo "$ac_file"|sed 's%:.*%%'` ;;
      esac
      # PO directories have a Makefile.in generated from Makefile.in.in.
      case "$ac_file" in */Makefile.in)
        # Adjust a relative srcdir.
        ac_dir=`echo "$ac_file"|sed 's%/[^/][^/]*$%%'`
        ac_dir_suffix="/`echo "$ac_dir"|sed 's%^\./%%'`"
        ac_dots=`echo "$ac_dir_suffix"|sed 's%/[^/]*%../%g'`
        # In autoconf-2.13 it is called $ac_given_srcdir.
        # In autoconf-2.50 it is called $srcdir.
        test -n "$ac_given_srcdir" || ac_given_srcdir="$srcdir"
        case "$ac_given_srcdir" in
          .)  top_srcdir=`echo $ac_dots|sed 's%/$%%'` ;;
          /*) top_srcdir="$ac_given_srcdir" ;;
          *)  top_srcdir="$ac_dots$ac_given_srcdir" ;;
        esac
        if test -f "$ac_given_srcdir/$ac_dir/POTFILES.in"; then
          rm -f "$ac_dir/POTFILES"
          test -n "$as_me" && echo "$as_me: creating $ac_dir/POTFILES" || echo "creating $ac_dir/POTFILES"
          cat "$ac_given_srcdir/$ac_dir/POTFILES.in" | sed -e "/^#/d" -e "/^[ 	]*\$/d" -e "s,.*,     $top_srcdir/& \\\\," | sed -e "\$s/\(.*\) \\\\/\1/" > "$ac_dir/POTFILES"
          # ALL_LINGUAS, POFILES, GMOFILES, UPDATEPOFILES, DUMMYPOFILES depend
          # on $ac_dir but don't depend on user-specified configuration
          # parameters.
          if test -f "$ac_given_srcdir/$ac_dir/LINGUAS"; then
            # The LINGUAS file contains the set of available languages.
            if test -n "$ALL_LINGUAS"; then
              test -n "$as_me" && echo "$as_me: setting ALL_LINGUAS in configure.in is obsolete" || echo "setting ALL_LINGUAS in configure.in is obsolete"
            fi
            ALL_LINGUAS_=`sed -e "/^#/d" "$ac_given_srcdir/$ac_dir/LINGUAS"`
            # Hide the ALL_LINGUAS assigment from automake.
            eval 'ALL_LINGUAS''=$ALL_LINGUAS_'
          fi
          case "$ac_given_srcdir" in
            .) srcdirpre= ;;
            *) srcdirpre='$(srcdir)/' ;;
          esac
          POFILES=
          GMOFILES=
          UPDATEPOFILES=
          DUMMYPOFILES=
          for lang in $ALL_LINGUAS; do
            POFILES="$POFILES $srcdirpre$lang.po"
            GMOFILES="$GMOFILES $srcdirpre$lang.gmo"
            UPDATEPOFILES="$UPDATEPOFILES $lang.po-update"
            DUMMYPOFILES="$DUMMYPOFILES $lang.nop"
          done
          # CATALOGS depends on both $ac_dir and the user's LINGUAS
          # environment variable.
          INST_LINGUAS=
          if test -n "$ALL_LINGUAS"; then
            for presentlang in $ALL_LINGUAS; do
              useit=no
              if test "%UNSET%" != "$LINGUAS"; then
                desiredlanguages="$LINGUAS"
              else
                desiredlanguages="$ALL_LINGUAS"
              fi
              for desiredlang in $desiredlanguages; do
                # Use the presentlang catalog if desiredlang is
                #   a. equal to presentlang, or
                #   b. a variant of presentlang (because in this case,
                #      presentlang can be used as a fallback for messages
                #      which are not translated in the desiredlang catalog).
                case "$desiredlang" in
                  "$presentlang"*) useit=yes;;
                esac
              done
              if test $useit = yes; then
                INST_LINGUAS="$INST_LINGUAS $presentlang"
              fi
            done
          fi
          CATALOGS=
          if test -n "$INST_LINGUAS"; then
            for lang in $INST_LINGUAS; do
              CATALOGS="$CATALOGS $lang.gmo"
            done
          fi
          test -n "$as_me" && echo "$as_me: creating $ac_dir/Makefile" || echo "creating $ac_dir/Makefile"
          sed -e "/^POTFILES =/r $ac_dir/POTFILES" -e "/^# Makevars/r $ac_given_srcdir/$ac_dir/Makevars" -e "s|@POFILES@|$POFILES|g" -e "s|@GMOFILES@|$GMOFILES|g" -e "s|@UPDATEPOFILES@|$UPDATEPOFILES|g" -e "s|@DUMMYPOFILES@|$DUMMYPOFILES|g" -e "s|@CATALOGS@|$CATALOGS|g" "$ac_dir/Makefile.in" > "$ac_dir/Makefile"
          for f in "$ac_given_srcdir/$ac_dir"/Rules-*; do
            if test -f "$f"; then
              case "$f" in
                *.orig | *.bak | *~) ;;
                *) cat "$f" >> "$ac_dir/Makefile" ;;
              esac
            fi
          done
        fi
        ;;
      esac
    done],
   [# Capture the value of obsolete ALL_LINGUAS because we need it to compute
    # POFILES, GMOFILES, UPDATEPOFILES, DUMMYPOFILES, CATALOGS. But hide it
    # from automake.
    eval 'ALL_LINGUAS''="$ALL_LINGUAS"'
    # Capture the value of LINGUAS because we need it to compute CATALOGS.
    LINGUAS="${LINGUAS-%UNSET%}"
   ])
])


dnl Checks for all prerequisites of the intl subdirectory,
dnl except for INTL_LIBTOOL_SUFFIX_PREFIX (and possibly LIBTOOL), INTLOBJS,
dnl            USE_INCLUDED_LIBINTL, BUILD_INCLUDED_LIBINTL.
AC_DEFUN([AM_INTL_SUBDIR],
[
  AC_REQUIRE([AC_PROG_INSTALL])dnl
  AC_REQUIRE([AM_MKINSTALLDIRS])dnl
  AC_REQUIRE([AC_PROG_CC])dnl
  AC_REQUIRE([AC_CANONICAL_HOST])dnl
  AC_REQUIRE([AC_PROG_RANLIB])dnl
  AC_REQUIRE([AC_ISC_POSIX])dnl
  AC_REQUIRE([AC_HEADER_STDC])dnl
  AC_REQUIRE([AC_C_CONST])dnl
  AC_REQUIRE([AC_C_INLINE])dnl
  AC_REQUIRE([AC_TYPE_OFF_T])dnl
  AC_REQUIRE([AC_TYPE_SIZE_T])dnl
  AC_REQUIRE([AC_FUNC_ALLOCA])dnl
  AC_REQUIRE([AC_FUNC_MMAP])dnl
  AC_REQUIRE([jm_GLIBC21])dnl
  AC_REQUIRE([gt_INTDIV0])dnl
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])dnl
  AC_REQUIRE([gt_INTTYPES_PRI])dnl

  AC_CHECK_HEADERS([argz.h limits.h locale.h nl_types.h malloc.h stddef.h \
stdlib.h string.h unistd.h sys/param.h])
  AC_CHECK_FUNCS([feof_unlocked fgets_unlocked getc_unlocked getcwd getegid \
geteuid getgid getuid mempcpy munmap putenv setenv setlocale stpcpy \
strcasecmp strdup strtoul tsearch __argz_count __argz_stringify __argz_next])

  AM_ICONV
  AM_LANGINFO_CODESET
  if test $ac_cv_header_locale_h = yes; then
    AM_LC_MESSAGES
  fi

  dnl intl/plural.c is generated from intl/plural.y. It requires bison,
  dnl because plural.y uses bison specific features. It requires at least
  dnl bison-1.26 because earlier versions generate a plural.c that doesn't
  dnl compile.
  dnl bison is only needed for the maintainer (who touches plural.y). But in
  dnl order to avoid separate Makefiles or --enable-maintainer-mode, we put
  dnl the rule in general Makefile. Now, some people carelessly touch the
  dnl files or have a broken "make" program, hence the plural.c rule will
  dnl sometimes fire. To avoid an error, defines BISON to ":" if it is not
  dnl present or too old.
  AC_CHECK_PROGS([INTLBISON], [bison])
  if test -z "$INTLBISON"; then
    ac_verc_fail=yes
  else
    dnl Found it, now check the version.
    AC_MSG_CHECKING([version of bison])
changequote(<<,>>)dnl
    ac_prog_version=`$INTLBISON --version 2>&1 | sed -n 's/^.*GNU Bison.* \([0-9]*\.[0-9.]*\).*$/\1/p'`
    case $ac_prog_version in
      '') ac_prog_version="v. ?.??, bad"; ac_verc_fail=yes;;
      1.2[6-9]* | 1.[3-9][0-9]* | [2-9].*)
changequote([,])dnl
         ac_prog_version="$ac_prog_version, ok"; ac_verc_fail=no;;
      *) ac_prog_version="$ac_prog_version, bad"; ac_verc_fail=yes;;
    esac
    AC_MSG_RESULT([$ac_prog_version])
  fi
  if test $ac_verc_fail = yes; then
    INTLBISON=:
  fi
])


AC_DEFUN([AM_MKINSTALLDIRS],
[
  dnl If the AC_CONFIG_AUX_DIR macro for autoconf is used we possibly
  dnl find the mkinstalldirs script in another subdir but $(top_srcdir).
  dnl Try to locate is.
  MKINSTALLDIRS=
  if test -n "$ac_aux_dir"; then
    MKINSTALLDIRS="$ac_aux_dir/mkinstalldirs"
  fi
  if test -z "$MKINSTALLDIRS"; then
    MKINSTALLDIRS="\$(top_srcdir)/mkinstalldirs"
  fi
  AC_SUBST(MKINSTALLDIRS)
])


dnl Usage: AM_GNU_GETTEXT_VERSION([gettext-version])
AC_DEFUN([AM_GNU_GETTEXT_VERSION], [])

# progtest.m4 serial 2 (gettext-0.10.40)
dnl Copyright (C) 1996-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.
dnl
dnl This file can can be used in projects which are not available under
dnl the GNU General Public License or the GNU Library General Public
dnl License but which still want to provide support for the GNU gettext
dnl functionality.
dnl Please note that the actual code of the GNU gettext library is covered
dnl by the GNU Library General Public License, and the rest of the GNU
dnl gettext package package is covered by the GNU General Public License.
dnl They are *not* in the public domain.

dnl Authors:
dnl   Ulrich Drepper <drepper@cygnus.com>, 1996.

# Search path for a program which passes the given test.

dnl AM_PATH_PROG_WITH_TEST(VARIABLE, PROG-TO-CHECK-FOR,
dnl   TEST-PERFORMED-ON-FOUND_PROGRAM [, VALUE-IF-NOT-FOUND [, PATH]])
AC_DEFUN([AM_PATH_PROG_WITH_TEST],
[# Extract the first word of "$2", so it can be a program name with args.
set dummy $2; ac_word=[$]2
AC_MSG_CHECKING([for $ac_word])
AC_CACHE_VAL(ac_cv_path_$1,
[case "[$]$1" in
  /*)
  ac_cv_path_$1="[$]$1" # Let the user override the test with a path.
  ;;
  *)
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:"
  for ac_dir in ifelse([$5], , $PATH, [$5]); do
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/$ac_word; then
      if [$3]; then
	ac_cv_path_$1="$ac_dir/$ac_word"
	break
      fi
    fi
  done
  IFS="$ac_save_ifs"
dnl If no 4th arg is given, leave the cache variable unset,
dnl so AC_PATH_PROGS will keep looking.
ifelse([$4], , , [  test -z "[$]ac_cv_path_$1" && ac_cv_path_$1="$4"
])dnl
  ;;
esac])dnl
$1="$ac_cv_path_$1"
if test ifelse([$4], , [-n "[$]$1"], ["[$]$1" != "$4"]); then
  AC_MSG_RESULT([$]$1)
else
  AC_MSG_RESULT(no)
fi
AC_SUBST($1)dnl
])

# intdiv0.m4 serial 1 (gettext-0.11.3)
dnl Copyright (C) 2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl From Bruno Haible.

AC_DEFUN([gt_INTDIV0],
[
  AC_REQUIRE([AC_PROG_CC])dnl
  AC_REQUIRE([AC_CANONICAL_HOST])dnl

  AC_CACHE_CHECK([whether integer division by zero raises SIGFPE],
    gt_cv_int_divbyzero_sigfpe,
    [
      AC_TRY_RUN([
#include <stdlib.h>
#include <signal.h>

static void
#ifdef __cplusplus
sigfpe_handler (int sig)
#else
sigfpe_handler (sig) int sig;
#endif
{
  /* Exit with code 0 if SIGFPE, with code 1 if any other signal.  */
  exit (sig != SIGFPE);
}

int x = 1;
int y = 0;
int z;
int nan;

int main ()
{
  signal (SIGFPE, sigfpe_handler);
/* IRIX and AIX (when "xlc -qcheck" is used) yield signal SIGTRAP.  */
#if (defined (__sgi) || defined (_AIX)) && defined (SIGTRAP)
  signal (SIGTRAP, sigfpe_handler);
#endif
/* Linux/SPARC yields signal SIGILL.  */
#if defined (__sparc__) && defined (__linux__)
  signal (SIGILL, sigfpe_handler);
#endif

  z = x / y;
  nan = y / y;
  exit (1);
}
], gt_cv_int_divbyzero_sigfpe=yes, gt_cv_int_divbyzero_sigfpe=no,
        [
          # Guess based on the CPU.
          case "$host_cpu" in
            alpha* | i[34567]86 | m68k | s390*)
              gt_cv_int_divbyzero_sigfpe="guessing yes";;
            *)
              gt_cv_int_divbyzero_sigfpe="guessing no";;
          esac
        ])
    ])
  case "$gt_cv_int_divbyzero_sigfpe" in
    *yes) value=1;;
    *) value=0;;
  esac
  AC_DEFINE_UNQUOTED(INTDIV0_RAISES_SIGFPE, $value,
    [Define if integer division by zero raises signal SIGFPE.])
])

# lcmessage.m4 serial 3 (gettext-0.11.3)
dnl Copyright (C) 1995-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.
dnl
dnl This file can can be used in projects which are not available under
dnl the GNU General Public License or the GNU Library General Public
dnl License but which still want to provide support for the GNU gettext
dnl functionality.
dnl Please note that the actual code of the GNU gettext library is covered
dnl by the GNU Library General Public License, and the rest of the GNU
dnl gettext package package is covered by the GNU General Public License.
dnl They are *not* in the public domain.

dnl Authors:
dnl   Ulrich Drepper <drepper@cygnus.com>, 1995.

# Check whether LC_MESSAGES is available in <locale.h>.

AC_DEFUN([AM_LC_MESSAGES],
[
  AC_CACHE_CHECK([for LC_MESSAGES], am_cv_val_LC_MESSAGES,
    [AC_TRY_LINK([#include <locale.h>], [return LC_MESSAGES],
       am_cv_val_LC_MESSAGES=yes, am_cv_val_LC_MESSAGES=no)])
  if test $am_cv_val_LC_MESSAGES = yes; then
    AC_DEFINE(HAVE_LC_MESSAGES, 1,
      [Define if your <locale.h> file defines LC_MESSAGES.])
  fi
])

