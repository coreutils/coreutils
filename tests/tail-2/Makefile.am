# Make coreutils tests for "tail".			-*-Makefile-*-

# Copyright (C) 1999, 2000, 2001, 2002, 2003, 2006 Free Software
# Foundation, Inc.

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

TESTS = \
  append-only \
  tail-n0f \
  infloop-1 \
  big-4gb proc-ksyms start-middle assert assert-2

include $(top_srcdir)/tests/check.mk
