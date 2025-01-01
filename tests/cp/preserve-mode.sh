#!/bin/sh
# Check whether cp generates files with correct modes.

# Copyright (C) 2002-2025 Free Software Foundation, Inc.

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
print_ver_ cp

get_mode() { stat -c%f "$1"; }

umask 0022 || framework_failure_

#regular file test
touch a b || framework_failure_
chmod 600 b || framework_failure_
cp --no-preserve=mode b c || fail=1
test "$(get_mode a)" = "$(get_mode c)" || fail=1

#existing destination test
chmod 600 c || framework_failure_
cp --no-preserve=mode a b || fail=1
test "$(get_mode b)" = "$(get_mode c)" || fail=1

#directory test
mkdir d1 d2 || framework_failure_
chmod 705 d2 || framework_failure_
cp --no-preserve=mode -r d2 d3 || fail=1
test "$(get_mode d1)" = "$(get_mode d3)" || fail=1

#contradicting options test
rm -f a b || framework_failure_
touch a || framework_failure_
chmod 600 a || framework_failure_
cp --no-preserve=mode --preserve=all a b || fail=1
test "$(get_mode a)" = "$(get_mode b)" || fail=1

#fifo test
if mkfifo fifo; then
  cp -a --no-preserve=mode fifo fifo_copy || fail=1
  #ensure default perms set appropriate for non regular files
  #which wasn't done between v8.20 and 8.29 inclusive
  test "$(get_mode fifo)" = "$(get_mode fifo_copy)" || fail=1
fi

# Test that plain --preserve=ownership does not affect destination mode.
rm -f a b c || framework_failure_
touch a || framework_failure_
chmod 660 a || framework_failure_
cp a b || fail=1
cp --preserve=ownership a c || fail=1
test "$(get_mode b)" = "$(get_mode c)" || fail=1

Exit $fail
