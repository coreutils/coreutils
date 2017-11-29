#!/bin/sh
# ensure that cp's --no-preserve=mode works correctly

# Copyright (C) 2002-2017 Free Software Foundation, Inc.

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

get_mode() { ls -ld "$1" | cut -b-10; }

rm -f a b c
umask 0022
touch a
touch b
chmod 600 b

#regular file test
cp --no-preserve=mode b c || fail=1
test "$(get_mode a)" = "$(get_mode c)" || fail=1

rm -rf d1 d2 d3
mkdir d1 d2
chmod 705 d2

#directory test
cp --no-preserve=mode -r d2 d3 || fail=1
test "$(get_mode d1)" = "$(get_mode d3)" || fail=1

rm -f a b c
touch a
chmod 600 a

#contradicting options test
cp --no-preserve=mode --preserve=all a b || fail=1
test "$(get_mode a)" = "$(get_mode b)" || fail=1

Exit $fail
