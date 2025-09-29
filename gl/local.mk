# Make coreutils programs.                             -*-Makefile-*-
# This is included by the top-level Makefile.am.

## Copyright (C) 2024-2025 Free Software Foundation, Inc.

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
## along with this program.  If not, see <https://www.gnu.org/licenses/>.

EXTRA_DIST += \
gl/lib/buffer-lcm.c \
gl/lib/buffer-lcm.h \
gl/lib/cl-strtod.c \
gl/lib/cl-strtod.h \
gl/lib/cl-strtold.c \
gl/lib/dtimespec-bound.c \
gl/lib/dtimespec-bound.h \
gl/lib/fadvise.c \
gl/lib/fadvise.h \
gl/lib/fd-reopen.c \
gl/lib/fd-reopen.h \
gl/lib/heap.c \
gl/lib/heap.h \
gl/lib/mbbuf.c \
gl/lib/mbbuf.h \
gl/lib/rand-isaac.c \
gl/lib/rand-isaac.h \
gl/lib/randint.c \
gl/lib/randint.h \
gl/lib/randperm.c \
gl/lib/randperm.h \
gl/lib/randread.c \
gl/lib/randread.h \
gl/lib/root-dev-ino.c \
gl/lib/root-dev-ino.h \
gl/lib/skipchars.c \
gl/lib/skipchars.h \
gl/lib/smack.h \
gl/lib/strintcmp.c \
gl/lib/strnumcmp-in.h \
gl/lib/strnumcmp.c \
gl/lib/strnumcmp.h \
gl/lib/targetdir.c \
gl/lib/targetdir.h \
gl/lib/xdectoimax.c \
gl/lib/xdectoint.c \
gl/lib/xdectoint.h \
gl/lib/xdectoumax.c \
gl/lib/xfts.c \
gl/lib/xfts.h \
gl/local.mk \
gl/modules/buffer-lcm \
gl/modules/cl-strtod \
gl/modules/cl-strtold \
gl/modules/dtimespec-bound \
gl/modules/fadvise \
gl/modules/fadvise-tests \
gl/modules/fd-reopen \
gl/modules/heap \
gl/modules/link-tests.diff \
gl/modules/mbbuf \
gl/modules/randint \
gl/modules/randperm \
gl/modules/randread \
gl/modules/randread-tests \
gl/modules/rename-tests.diff \
gl/modules/root-dev-ino \
gl/modules/skipchars \
gl/modules/smack \
gl/modules/strnumcmp \
gl/modules/targetdir \
gl/modules/xdectoint \
gl/modules/xfts \
gl/tests/test-fadvise.c \
gl/tests/test-rand-isaac.c
