#!/bin/sh
# ensure that dd's iflag=direct and oflag=direct work

# Copyright (C) 2009-2026 Free Software Foundation, Inc.

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
print_ver_ dd

# dd "$1" to 'out' using Direct I/O with the current defined $block_size
dd_direct() {
  dd if="$1" of=out ibs=$block_size obs=$block_size iflag=direct oflag=direct
}

truncate -s 8192 in || framework_failure_

block_size=512
if ! dd_direct 'in' 2>/dev/null; then
  # Some file systems/devices require 4 KiB alignment for direct I/O.
  block_size=4096
  dd_direct 'in' 2>/dev/null ||
    skip_ 'neither 512B or 4KiB aligned DIRECT I/O succeeded'
fi

truncate -s $((block_size - 1)) short || framework_failure_
truncate -s 8191 m1 || framework_failure_
truncate -s 8193 p1 || framework_failure_

for i in short m1 p1; do
  rm -f out
  dd_direct "$i" || fail=1
done

Exit $fail
