#!/bin/sh
# Ensure we handle i/o errors correctly in csplit via /dev/full

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
print_ver_ csplit
getlimits_

cp -sf /dev/full xx01 || skip_ '/dev/full is required'

# Create the expected error message
printf '%s\n' "csplit: xx01: $ENOSPC" > exp || framework_failure_

# the 'csplit' command should fail with exit code 1
seq 2 | returns_ 1 csplit - 1 2> err || fail=1
# csplit should cleanup broken files
test -e xx01 && fail=1

# Ensure we got the expected error message
compare exp err || fail=1

# csplit does not remove xx01 directory
mkdir xx01 || framework_failure_
seq 2 | returns_ 1 csplit - 1 || fail=1
test -d xx01 || fail=1

Exit $fail
