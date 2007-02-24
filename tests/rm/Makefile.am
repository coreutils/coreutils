# Make coreutils tests for "rm".			-*-Makefile-*-

# Copyright (C) 1997, 1998, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
# Free Software Foundation, Inc.

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
  i-never \
  fail-eacces \
  one-file-system \
  ignorable \
  readdir-bug \
  empty-inacc \
  dir-nonrecur \
  dot-rel \
  inaccessible \
  unread3 \
  no-give-up \
  dir-no-w \
  fail-2eperm \
  cycle i-no-r fail-eperm \
  dangling-symlink rm1 rm2 rm3 rm4 rm5 \
  unread2 r-1 r-2 r-3 r-4 i-1 ir-1 f-1 sunos-1 deep-1 hash \
  interactive-always interactive-once \
  isatty # unreadable empty-name
EXTRA_DIST = $(TESTS)
TESTS_ENVIRONMENT = \
  RM=../../src/rm \
  MKDIR=../../src/mkdir \
  CONFIG_HEADER=$(CONFIG_HEADER) \
  top_srcdir=$(top_srcdir) \
  srcdir=$(srcdir) \
  PERL="$(PERL)" \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH" \
  PACKAGE_VERSION=$(PACKAGE_VERSION) \
  PROG=rm
