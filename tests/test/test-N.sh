#!/bin/sh
# Test 'test -N file'.

# Copyright (C) 2018-2025 Free Software Foundation, Inc.

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
print_ver_ test stat

stat_test_N() {
  mtime=$(env stat -c '%.Y' "$1")
  atime=$(env stat -c '%.X' "$1")
  test "$mtime" = "$atime" && return 1
  latest=$(printf '%s\n' "$mtime" "$atime" | sort -g | tail -n1)
  test "$mtime" = "$latest"
}

# For a freshly touched file, atime should equal mtime: 'test -N' returns 1.
touch file || framework_failure_
if ! stat_test_N file; then
  returns_ 1 env test -N file || fail=1
fi

# Set access time to 2 days ago: 'test -N' returns 0.
touch -a -d '12:00 today -2 days' file || framework_failure_
if stat_test_N file; then
  env test -N file || fail=1
fi

# Set mtime to 2 days before atime: 'test -N' returns 1;
touch -m -d '12:00 today -4 days' file || framework_failure_
if ! stat_test_N file; then
  returns_ 1 env test -N file || fail=1
fi

# Now modify the file: 'test -N' returns 0.
echo 'data' > file || framework_failure_
if stat_test_N file; then
  env test -N file || fail=1
fi

Exit $fail
