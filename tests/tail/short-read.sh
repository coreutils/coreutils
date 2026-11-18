#!/bin/sh
# Exercise positive short reads in tail's backward scan.

# Copyright (C) 2026 Free Software Foundation, Inc.

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

file=/sys/kernel/btf/vmlinux
test -r "$file" || skip_ "cannot read $file"

# The BTF string table ends in NUL-terminated names and has few newlines.
# Check both the final block and a scan that reaches earlier blocks.
for opt in -zn1 -n10; do
  tail ---presume-input-pipe "$opt" "$file" > exp || framework_failure_
  tail "$opt" "$file" > out || fail=1
  compare exp out || fail=1
done

Exit $fail
