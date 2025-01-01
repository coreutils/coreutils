#!/bin/sh
# sysfs files have weird properties that can be influenced by page size

# Copyright 2023-2025 Free Software Foundation, Inc.

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
print_ver_ tail

file='/sys/kernel/profiling'

test -r "$file" || skip_ "No $file file"

cp -f "$file" exp || framework_failure_

tail -n1 "$file" > out || fail=1
compare exp out || fail=1

tail -c2 "$file" > out || fail=1
compare exp out || fail=1

Exit $fail
