# Make coreutils tests for "du".			-*-Makefile-*-

# Copyright (C) 2000, 2002-2007 Free Software Foundation, Inc.

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

TESTS = \
  one-file-system \
  inacc-dest \
  long-from-unreadable \
  long-sloop \
  2g \
  files0-from \
  inaccessible-cwd \
  deref-args \
  slash \
  fd-leak \
  hard-link 8gb basic restore-wd \
  exclude no-x no-deref trailing-slash deref two-args slink
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  PERL="$(PERL)" \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH"
