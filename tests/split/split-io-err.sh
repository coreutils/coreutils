#!/bin/sh
# Ensure we handle i/o errors correctly in split via /dev/full

# Copyright (C) 2025-2026 Free Software Foundation, Inc.

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
print_ver_ split
getlimits_

cp -sf /dev/full xaa || skip_ '/dev/full is required'

# Create the expected error message
printf '%s\n' "split: xaa: $ENOSPC" > exp || framework_failure_

# the 'split' command should fail with exit code 1
seq 2 | returns_ 1 split -b 1 2> err || fail=1
# split does not cleanup broken file (while csplit does)
test -e xaa || fail=1
# split should not continue
test -e xab && fail=1

# Ensure we got the expected error message
compare exp err || fail=1

rm xaa || framework_failure_
# Similar for directory
mkdir xaa || framework_failure_
seq 2 | returns_ 1 split -b 1 2 || fail=1
test -d xaa || fail=1

Exit $fail
