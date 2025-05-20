#!/bin/sh
# From 7.2-9.7, this would trigger an out of bounds mem read

# Copyright (C) 2025 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ sort
getlimits_

# This issue triggers with valgrind or ASAN
valgrind --error-exitcode=1 sort --version 2>/dev/null &&
  VALGRIND='valgrind --error-exitcode=1'

{ printf '%s\n' aa bb; } > in || framework_failure_

_POSIX2_VERSION=200809 $VALGRIND sort +0.${SIZE_MAX}R in > out || fail=1
compare in out || fail=1

_POSIX2_VERSION=200809 $VALGRIND sort +1 -1.${SIZE_MAX}R in > out || fail=1
compare in out || fail=1

Exit $fail
