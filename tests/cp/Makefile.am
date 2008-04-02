# Make coreutils tests for cp.				-*-Makefile-*-

# Copyright (C) 1997-2001, 2003, 2005-2008 Free Software Foundation, Inc.

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

TESTS = \
  special-f \
  parent-perm \
  abuse \
  proc-zero-len \
  thru-dangling \
  cp-a-selinux \
  file-perm-race parent-perm-race \
  existing-perm-race \
  backup-dir \
  src-base-dot \
  sparse \
  link-no-deref \
  cp-deref \
  acl \
  preserve-2 r-vs-symlink link-preserve preserve-gid \
  backup-1 no-deref-link1 no-deref-link2 no-deref-link3 backup-is-src \
  same-file cp-mv-backup symlink-slash slink-2-slink fail-perm dir-slash \
  perm cp-HL cp-i special-bits link dir-rm-dest cp-parents deref-slink \
  dir-vs-file into-self
EXTRA_DIST = $(TESTS) trailing-slash

include $(top_srcdir)/tests/check.mk
