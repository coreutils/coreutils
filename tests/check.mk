# Include this file at the end of each tests/*/Makefile.am.
# Copyright (C) 2007, 2008 Free Software Foundation, Inc.

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
vc_exe_in_TESTS: Makefile
	@rm -f t1 t2
	@if test -d $(top_srcdir)/.git && test $(srcdir) = .; then	\
	  sed -n '/^$(_v) = \\$$/,/[^\]$$/p' $(srcdir)/Makefile.am	\
	    | sed 's/^  *//;/^\$$.*/d;/^$(_v) =/d'			\
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

# Append this, because automake does the same.
TESTS_ENVIRONMENT =				\
  abs_top_builddir='$(abs_top_builddir)'	\
  abs_top_srcdir='$(abs_top_srcdir)'		\
  built_programs="`$(built_programs)`"		\
  host_os=$(host_os)				\
  host_triplet='$(host_triplet)'		\
  srcdir='$(srcdir)'				\
  top_srcdir='$(top_srcdir)'			\
  CONFIG_HEADER='$(abs_top_builddir)/lib/config.h' \
  CU_TEST_NAME=`basename '$(abs_srcdir)'`,$$tst	\
  EGREP='$(EGREP)'				\
  EXEEXT='$(EXEEXT)'				\
  MAKE=$(MAKE)					\
  PACKAGE_BUGREPORT='$(PACKAGE_BUGREPORT)'	\
  PACKAGE_VERSION=$(PACKAGE_VERSION)		\
  PERL='$(PERL)'				\
  REPLACE_GETCWD=$(REPLACE_GETCWD)		\
  PATH='$(abs_top_builddir)/src$(PATH_SEPARATOR)'"$$PATH"

TEST_LOGS = $(TESTS:=.log)

# Parallel replacement of Automake's check-TESTS target.
include $(top_srcdir)/build-aux/check.mk

VERBOSE = yes
