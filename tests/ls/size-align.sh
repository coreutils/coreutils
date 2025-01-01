#!/bin/sh
# test size alignment

# Copyright (C) 2023-2025 Free Software Foundation, Inc.

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
print_ver_ ls truncate

truncate -s      0 small || framework_failure_
truncate -s 123456 large || framework_failure_
echo > alloc || framework_failure_

ls -s -l small alloc large > out || fail=1
len=$(wc -L < out) || framework_failure_
lines=$(wc -l < out) || framework_failure_
same=$(grep "^.\\{$len\\}\$" out | wc -l) || framework_failure_
test "$same" = "$lines" || { cat out; fail=1; }

Exit $fail
