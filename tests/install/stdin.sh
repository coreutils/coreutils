#!/bin/sh
# Test 'install' when the source is standard input.

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
print_ver_ ginstall

# work around a dash bug when redirecting
# from symlinked ttys in the background
tty=$(readlink -f /dev/stdin)

test -r "$tty" 2>&1 \
  || skip_ '/dev/stdin is not readable'

echo a >a || framework_failure_
echo b >b || framework_failure_

# Test the behavior when the source is a pipe and the destination does
# not exist.
cat a | ginstall /dev/stdin file1 >out 2>err || fail=1
compare a file1 || fail=1
compare /dev/null out || fail=1
compare /dev/null err || fail=1

# Test the behavior when the source is a file and the destination does
# not exist.
ginstall /dev/stdin file2 <b >out 2>err || fail=1
compare b file2 || fail=1
compare /dev/null out || fail=1
compare /dev/null err || fail=1

# Test the behavior when the source is a file and the destination exists.
ginstall /dev/stdin file1 <file2 >out 2>err || fail=1
compare b file1 || fail=1
compare /dev/null out || fail=1
compare /dev/null err || fail=1

# Test the behavior when the source is a pipe and the destination exists.
cat b | ginstall /dev/stdin file1 >out 2>err || fail=1
compare b file1 || fail=1
compare /dev/null out || fail=1
compare /dev/null err || fail=1

Exit $fail
