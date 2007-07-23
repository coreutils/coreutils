## Makefile for gnulib/lib				-*-Makefile-*-

# Copyright (C) 1995-2006 Free Software Foundation, Inc.

## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

include gnulib.mk

AM_CFLAGS = $(WARNING_CFLAGS) $(WERROR_CFLAGS)

noinst_PROGRAMS = t-fpending
LDADD = $(noinst_LIBRARIES)

libcoreutils_a_SOURCES += \
  buffer-lcm.c buffer-lcm.h \
  xmemxfrm.c xmemxfrm.h

libcoreutils_a_LIBADD += $(LIBOBJS)
libcoreutils_a_DEPENDENCIES += $(LIBOBJS)

check: unit-test

.PHONY: unit-test
unit-test: t-fpending
	./t-fpending > /dev/null
