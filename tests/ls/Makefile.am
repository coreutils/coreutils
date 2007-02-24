# Make coreutils tests for "ls".		-*-Makefile-*-

# Copyright (C) 1997-2003, 2006-2007 Free Software Foundation, Inc.

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
  nameless-uid \
  color-dtype-dir \
  stat-failed \
  stat-dtype \
  inode dangle file-type recursive dired infloop \
  rt-1 time-1 symlink-slash follow-slink no-arg m-option \
  stat-vs-dirent

EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  top_srcdir=$(top_srcdir) \
  srcdir=$(srcdir) \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH" \
  PERL="$(PERL)" \
  PROG=ls
