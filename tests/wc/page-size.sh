#!/bin/sh
# Test 'wc' on files with a size that is a multiple of the system's page size.

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
print_ver_ wc

# Get the systems actual page size if possible. Otherwise 4096 is good enough.
page_size=$(getconf PAGESIZE || echo 4096)

for multiple in $(seq 3); do
  truncate -s +$page_size file || framework_failure_
  # Count the entire file as a sanity check.
  wc -c < file > out 2> err || fail=1
  file_size=$(($multiple * $page_size))
  echo $file_size > exp || framework_failure_

  compare exp out || fail=1
  compare /dev/null err || fail=1
  # Test 'wc -c' when standard input has a nonzero offset.
  # This would give an incorrect result from coreutils-8.24
  # to coreutils-9.12.
  skip=$(($multiple * 100))
  (head -c $skip >/dev/null; wc -c > out 2> err) < file || fail=1
  echo $(($file_size - $skip)) > exp || framework_failure_

  compare exp out || fail=1
  compare /dev/null err || fail=1
  # Likewise.
  (head -c $page_size >/dev/null; wc -c > out 2> err) < file || fail=1
  echo $(($file_size - $page_size)) > exp || framework_failure_

  compare exp out || fail=1
  compare /dev/null err || fail=1
done

Exit $fail
