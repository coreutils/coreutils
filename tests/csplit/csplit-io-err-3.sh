#!/bin/sh
# Ensure we handle i/o errors correctly in csplit via dmsetup

# Copyright (C) 2015-2025 Free Software Foundation, Inc.

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

require_root_
dmsetup create ioerr --table "0 8 error" || skip_ 'dmsetup is required'
cleanup_() { dmsetup remove ioerr ; }
cp -sf /dev/mapper/ioerr xx01 || framework_failure_

# Get the wording of the OS-dependent EIO message
returns_ 1 seq 1 >/dev/mapper/ioerr 2>msgt || framework_failure_
sed 's/seq: write error: //' msgt > msg || framework_failure_

# Create the expected error message
{ printf "%s" "csplit: write error for 'xx01': " ; cat msg ; } > exp \
  || framework_failure_

# Force write error
# the 'csplit' command should fail with exit code 1
# (checked with 'returns_ 1 ... || fail=1')
seq 1000 | (returns_ 1 csplit - 1 >err) || fail=1
# csplit should cleanup broken files
test -e xx01 && fail=1

# Ensure we got the expected error message
compare exp err || fail=1

Exit $fail
