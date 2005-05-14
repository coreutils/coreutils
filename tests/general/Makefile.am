## Process this file with automake to create Makefile.in.

## Makefile for Autoconf testsuite.
## Copyright 2000, 2003 Free Software Foundation, Inc.

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
## 02110-1301, USA.

AUTOMAKE_OPTIONS = gnits

SUITE = suite.at dd.at m4sh.at

EXTRA_DIST = $(SUITE) testsuite

TESTS_ENVIRONMENT = \
  PATH="`pwd`/../../src$(PATH_SEPARATOR)$$PATH"

TESTS = testsuite

check-local: atconfig

testsuite: $(SUITE) atgeneral.m4
	$(M4) -I $(srcdir) atgeneral.m4 suite.at > $@-t1
	sed 's/[	 ]*$$//;/^$$/N;/\n$$/D' $@-t1 > $@-tmp
	rm -f $@-t1
	chmod +x $@-tmp
	mv $@-tmp $@

DISTCLEANFILES = atconfig
