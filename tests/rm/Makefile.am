# Make coreutils tests for "rm".			-*-Makefile-*-

# Copyright (C) 1997-1998, 2000-2008 Free Software Foundation, Inc.

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
  deep-2 \
  deep-1 \
  hash \
  dangling-symlink \
  v-slash \
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
  empty-name \
  fail-2eperm \
  cycle i-no-r fail-eperm \
  rm1 rm2 rm3 rm4 rm5 \
  unread2 r-1 r-2 r-3 r-4 i-1 ir-1 f-1 sunos-1 \
  unreadable \
  interactive-always interactive-once \
  isatty

EXTRA_DIST = $(TESTS)

include $(top_srcdir)/tests/check.mk
