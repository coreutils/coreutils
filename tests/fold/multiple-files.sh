#!/bin/sh
# Test fold with multiple files.

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
print_ver_ fold

# Test that all files are processed.
echo a > file1
echo b > file2
returns_ 1 fold file1 missing file2 > out 2> err || fail=1
cat <<EOF > exp-out || framework_failure_
a
b
EOF
cat <<EOF > exp-err || framework_failure_
fold: missing: No such file or directory
EOF
compare exp-out out || fail=1
compare exp-err err || fail=1

Exit $fail
