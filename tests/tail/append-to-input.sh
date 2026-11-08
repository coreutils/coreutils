#!/bin/sh
# Ensure tail stops at the original end of a file appended to its output

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
print_ver_ tail seq

# Cap the file size so a runaway tail cannot fill the file system.
(ulimit -f 10000) 2>/dev/null || skip_ 'unable to limit file size'

# A file larger than a buffer, read from the end.
for opt in -n1 -c4; do
  seq 100000 > in || framework_failure_
  cp in exp || framework_failure_
  tail $opt in >> exp || framework_failure_
  (ulimit -f 10000; timeout 10 tail $opt in >> in) || fail=1
  compare exp in || fail=1
done

# A small file, read from the start.
for opt in -n+2 -c+3; do
  printf 'a\nb\nc\n' > in || framework_failure_
  printf 'a\nb\nc\nb\nc\n' > exp || framework_failure_
  (ulimit -f 10000; timeout 10 tail $opt in >> in) || fail=1
  compare exp in || fail=1
done

Exit $fail
