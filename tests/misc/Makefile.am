# Make miscellaneous coreutils tests.			-*-Makefile-*-

# Copyright (C) 2001-2007 Free Software Foundation, Inc.

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

EXTRA_DIST = $(TESTS)

built_programs = \
  (cd $(top_builddir)/src && MAKEFLAGS= $(MAKE) -s built_programs.list)

TESTS_ENVIRONMENT = \
  built_programs="`$(built_programs)`" \
  PACKAGE_VERSION=$(PACKAGE_VERSION) \
  PERL="$(PERL)" \
  BUILD_SRC_DIR="`pwd`/../../src" \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH" \
  CONFIG_HEADER=$(CONFIG_HEADER) \
  REPLACE_GETCWD=$(REPLACE_GETCWD) \
  host_os=$(host_os)

# Do not choose a name that is a shell keyword like 'if', or a
# commonly-used utility like 'cat' or 'test', as the name of a test.
# Otherwise, VPATH builds will fail on hosts like Solaris, since they
# will expand 'if test ...' to 'if .../test ...', and the '.../test'
# will execute the test script rather than the standard utility.

TESTS = \
  head-elide-tail \
  ls-time \
  ls-misc \
  date \
  xstrtol \
  od \
  arch \
  pr \
  df-P \
  pwd-unreadable-parent \
  chcon \
  chcon-fail \
  selinux \
  cut \
  wc-files0-from \
  wc-files0 \
  cat-proc \
  base64 \
  basename \
  close-stdout \
  csplit \
  date-sec \
  dircolors \
  df \
  dirname \
  expand \
  expr \
  factor \
  false-status \
  fmt \
  fmt-long-line \
  fold \
  groups-version \
  head-c \
  head-pos \
  md5sum \
  md5sum-newline \
  mknod \
  nice \
  nl \
  nohup \
  od-N \
  od-x8 \
  paste-no-nl \
  pathchk1 \
  printf \
  printf-hex \
  pwd-long \
  runcon-no-reorder \
  seq \
  sha1sum \
  sha1sum-vec \
  sha224sum \
  sha256sum \
  sha384sum \
  sha512sum \
  shred-exact \
  shred-remove \
  shuf \
  sort-compress \
  sort-merge \
  sort-rand \
  split-a \
  split-fail \
  split-l \
  stat-fmt \
  stat-printf \
  stty \
  stty-invalid \
  stty-row-col \
  sum \
  sum-sysv \
  tac-continue \
  tee \
  tee-dash \
  test-diag \
  tsort \
  tty-eof \
  unexpand


include $(top_srcdir)/tests/check.mk
