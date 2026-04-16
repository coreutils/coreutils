#!/bin/sh
# Test the behavior of 'mktemp' when it fails to write the file name
# to standard output.

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
print_ver_ mktemp

if ! test -w /dev/full || ! test -c /dev/full; then
  skip_ '/dev/full is required'
fi

mkdir -p a/b || framework_failure_
returns_ 1 mktemp -p a/b > /dev/full || fail=1
returns_ 1 mktemp -p a/b -d > /dev/full || fail=1

# Check that the directory is empty.
for file in a/b/*; do
  test -e "$file" && fail=1
done

Exit $fail
