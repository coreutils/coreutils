# Make coreutils tests for "mv".			-*-Makefile-*-

# Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
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
  hard-verbose \
  backup-dir \
  dir2dir \
  no-target-dir \
  atomic \
  acl \
  reply-no \
  leak-fd \
  part-hardlink \
  hard-4 \
  hard-3 \
  hard-2 \
  perm-1 \
  i-link-no \
  part-fail \
  dup-source childproof i-4 i-5 update i-2 mv-special-1 \
  into-self into-self-2 into-self-3 into-self-4 \
  backup-is-src \
  i-1 hard-link-1 force partition-perm to-symlink dir-file diag \
  part-symlink part-rename trailing-slash

EXTRA_DIST = $(TESTS) vfat
TESTS_ENVIRONMENT = \
  PERL="$(PERL)" \
  EGREP="$(EGREP)" \
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH" \
  CONFIG_HEADER=$(CONFIG_HEADER) \
  PROG=mv
