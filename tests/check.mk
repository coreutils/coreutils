# Include this file at the end of each tests/*/Makefile.am.
# Copyright (C) 2007 Free Software Foundation, Inc.

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
vc_exe_in_TESTS: Makefile
	@if test -d $(top_srcdir)/.git; then \
	  echo $(TESTS) | tr -s ' ' '\n' | sort -u > t1; \
	  for f in `$(top_srcdir)/build-aux/vc-list-files .`; do \
	    test -f "$$f" && test -x "$$f" && echo "$$f"; \
	  done | sort -u | diff -u t1 -; \
	else :; fi

check: vc_exe_in_TESTS
.PHONY: vc_exe_in_TESTS

# Append this, because automake does the same.
TESTS_ENVIRONMENT +=			\
  top_srcdir=$(top_srcdir)		\
  abs_top_srcdir=$(abs_top_srcdir)	\
  abs_top_builddir=$(abs_top_builddir)	\
  srcdir=$(srcdir)

TEST_LOGS = $(TESTS:=.log)

# Parallel replacement of Automake's check-TESTS target.
# CAVEAT: code in the following relies on GNU make.
include $(top_srcdir)/build-aux/check.mk
