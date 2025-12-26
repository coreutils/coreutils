#!/bin/sh
# Ensure we handle i/o errors correctly in csplit

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

# Use root instead of LD_PRELOAD for static binary
require_root_
mkdir small || framework_failure_
mount -t tmpfs -o size=1k tmpfs small  || skip_ 'Unable to mount small tmpfs'

cleanup_() { umount small; rm -d small }

# Ensure error messages are in English
LC_ALL=C
export LC_ALL

# Get the wording of the OS-dependent ENOSPC message
returns_ 1 seq 1 >/dev/full 2>msgt || framework_failure_
sed 's/seq: write error: //' msgt > msg || framework_failure_

# Create the expected error message
{ printf "%s" "csplit: write error for 'xx01': " ; cat msg ; } > exp \
  || framework_failure_

# Force write error
# the 'csplit' command should fail with exit code 1
# (checked with 'returns_ 1 ... || fail=1')
( cd small && seq 2000 | (returns_ 1 csplit - 1 2>../out) ) || fail=1
# csplit should cleanup broken files
test -e small/xx01 && fail=1

# Ensure we got the expected error message
compare exp out || fail=1

Exit $fail
