# Include this file at the end of each tests/*/Makefile.am.
# Copyright (C) 2007-2009 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Ensure that all version-controlled executable files are listed in TESTS.
# Collect test names from the line matching /^TESTS = \\$$/ to the following
# one that does not end in '\'.
_v = TESTS
_w = root_tests
vc_exe_in_TESTS: Makefile
	@rm -f t1 t2
	@if test -d $(top_srcdir)/.git && test $(srcdir) = .; then	\
	  { sed -n '/^$(_v) =[	 ]*\\$$/,/[^\]$$/p'			\
		$(srcdir)/Makefile.am					\
	    | sed 's/^  *//;/^\$$.*/d;/^$(_v) =/d';			\
	    sed -n '/^$(_w) =[	 ]*\\$$/,/[^\]$$/p'			\
		$(srcdir)/Makefile.am					\
	    | sed 's/^  *//;/^\$$.*/d;/^$(_w) =/d'; }			\
	    | tr -s '\012\\' '  ' | fmt -1 | sort -u > t1 &&		\
	  for f in `cd $(top_srcdir) && build-aux/vc-list-files $(subdir)`; do \
	    f=`echo $$f|sed 's!^$(subdir)/!!'`;				\
	    test -f "$$f" && test -x "$$f" && echo "$$f";		\
	  done | sort -u > t2 &&					\
	  diff -u t1 t2 || exit 1;					\
	  rm -f t1 t2;							\
	else :; fi

check: vc_exe_in_TESTS
.PHONY: vc_exe_in_TESTS

built_programs = \
  (cd $(top_builddir)/src && MAKEFLAGS= $(MAKE) -s built_programs.list)

# Note that the first lines are statements.  They ensure that environment
# variables that can perturb tests are unset or set to expected values.
# The rest are envvar settings that propagate build-related Makefile
# variables to test scripts.
TESTS_ENVIRONMENT =				\
  . $(srcdir)/lang-default;			\
  tmp__=$$TMPDIR; test -d "$$tmp__" || tmp__=.;	\
  . $(srcdir)/envvar-check;			\
  TMPDIR=$$tmp__; export TMPDIR;		\
  exec 9>&2;					\
  shell_or_perl_() {				\
    if grep '^\#!/usr/bin/perl' "$$1" > /dev/null; then			\
      if $(PERL) -e 'use warnings' > /dev/null 2>&1; then		\
	grep '^\#!/usr/bin/perl -T' "$$1" > /dev/null && T_=T || T_=;	\
        $(PERL) -w$$T_ -I$(srcdir) -MCoreutils				\
	      -M"CuTmpdir qw($$f)" -- "$$1";	\
      else					\
	echo 1>&2 "$$tst: configure did not find a usable version of Perl," \
	  "so skipping this test";		\
	(exit 77);				\
      fi;					\
    else					\
      $(SHELL) "$$1";				\
    fi;						\
  };						\
  export					\
  LOCALE_FR='$(LOCALE_FR)'			\
  LOCALE_FR_UTF8='$(LOCALE_FR_UTF8)'		\
  abs_top_builddir='$(abs_top_builddir)'	\
  abs_top_srcdir='$(abs_top_srcdir)'		\
  abs_srcdir='$(abs_srcdir)'			\
  built_programs="`$(built_programs)`"		\
  host_os=$(host_os)				\
  host_triplet='$(host_triplet)'		\
  srcdir='$(srcdir)'				\
  top_srcdir='$(top_srcdir)'			\
  CONFIG_HEADER='$(abs_top_builddir)/lib/config.h' \
  CU_TEST_NAME=`basename '$(abs_srcdir)'`,$$tst	\
  CC='$(CC)'					\
  AWK='$(AWK)'					\
  EGREP='$(EGREP)'				\
  EXEEXT='$(EXEEXT)'				\
  MAKE=$(MAKE)					\
  PACKAGE_BUGREPORT='$(PACKAGE_BUGREPORT)'	\
  PACKAGE_VERSION=$(PACKAGE_VERSION)		\
  PERL='$(PERL)'				\
  PREFERABLY_POSIX_SHELL='$(PREFERABLY_POSIX_SHELL)' \
  REPLACE_GETCWD=$(REPLACE_GETCWD)		\
  PATH='$(abs_top_builddir)/src$(PATH_SEPARATOR)'"$$PATH" \
  ; shell_or_perl_

VERBOSE = yes
