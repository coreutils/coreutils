# Make coreutils tests for "touch".		-*-Makefile-*-

# Copyright (C) 1998-2007 Free Software Foundation, Inc.

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
  read-only \
  relative \
  not-owner \
  no-create-missing \
  fail-diag dir-1 dangling-symlink empty-file fifo no-rights obsolescent
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  host_triplet=$(host_triplet) \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH"
