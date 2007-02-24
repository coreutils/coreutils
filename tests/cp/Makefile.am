# Make coreutils tests for cp.				-*-Makefile-*-

# Copyright (C) 1997-2001, 2003, 2005-2007 Free Software Foundation, Inc.

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
  file-perm-race parent-perm-race \
  backup-dir \
  src-base-dot \
  sparse \
  link-no-deref \
  cp-deref \
  acl \
  preserve-2 r-vs-symlink link-preserve \
  backup-1 no-deref-link1 no-deref-link2 no-deref-link3 backup-is-src \
  same-file cp-mv-backup symlink-slash slink-2-slink fail-perm dir-slash \
  perm cp-HL cp-i special-bits link dir-rm-dest cp-parents deref-slink \
  dir-vs-file into-self
EXTRA_DIST = $(TESTS) trailing-slash
TESTS_ENVIRONMENT =			\
  MAKE=$(MAKE)				\
  abs_top_builddir=$(abs_top_builddir)	\
  CONFIG_HEADER=$(CONFIG_HEADER)	\
  CU_TEST_NAME=`basename $(abs_srcdir)`,$$tst \
  PATH="$(VG_PATH_PREFIX)`pwd`/../../src$(PATH_SEPARATOR)$$PATH"
