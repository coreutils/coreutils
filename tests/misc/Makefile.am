# Make miscellaneous coreutils tests.			-*-Makefile-*-

# Copyright (C) 200-2007 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

EXTRA_DIST = $(TESTS)

TESTS_ENVIRONMENT = \
  top_srcdir=$(top_srcdir) \
  abs_top_builddir=$(abs_top_builddir) \
  srcdir=$(srcdir) \
  PACKAGE_VERSION=$(PACKAGE_VERSION) \
  PERL="$(PERL)" \
  BUILD_SRC_DIR="`pwd`/../../src" \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH" \
  CONFIG_HEADER=$(CONFIG_HEADER) \
  REPLACE_GETCWD=$(REPLACE_GETCWD) \
  host_os=$(host_os) \
  PROG=`../../src/basename -- "$$tst"`

# Do not choose a name that is a shell keyword like 'if', or a
# commonly-used utility like 'cat' or 'test', as the name of a test.
# Otherwise, VPATH builds will fail on hosts like Solaris, since they
# will expand 'if test ...' to 'if .../test ...', and the '.../test'
# will execute the test script rather than the standard utility.

TESTS = \
  pr \
  df-P \
  pwd-unreadable-parent \
  cut \
  wc-files0-from \
  wc-files0 \
  cat-proc \
  base64 \
  basename \
  close-stdout \
  csplit \
  date \
  date-sec \
  df \
  dirname \
  expand \
  false-status \
  fold \
  head-c \
  head-elide-tail \
  head-pos \
  mknod \
  nice \
  nl \
  nohup \
  paste-no-nl \
  pathchk1 \
  printf \
  printf-hex \
  pwd-long \
  sha224sum \
  sha256sum \
  sha384sum \
  sha512sum \
  shuf \
  sort-compress \
  sort-merge \
  sort-rand \
  split-a \
  split-fail \
  split-l \
  stat-fmt \
  stat-printf \
  tac-continue \
  test-diag \
  tty-eof
