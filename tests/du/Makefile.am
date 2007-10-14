# Make coreutils tests for "du".			-*-Makefile-*-

# Copyright (C) 2000, 2002-2007 Free Software Foundation, Inc.

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
  2g \
  inacc-dir \
  one-file-system \
  inacc-dest \
  long-from-unreadable \
  long-sloop \
  files0-from \
  inaccessible-cwd \
  deref-args \
  slash \
  fd-leak \
  hard-link 8gb basic restore-wd \
  exclude no-x no-deref trailing-slash deref two-args slink
EXTRA_DIST = $(TESTS)

include $(top_srcdir)/tests/check.mk
