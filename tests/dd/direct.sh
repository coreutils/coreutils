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

truncate -s 8192 in || framework_failure_
block_size=512
if ! dd if=in of=out ibs=$block_size obs=$block_size oflag=direct 2> /dev/null; then
  # Some filesystems require 4 KiB alignment for direct I/O.
  block_size=4096
  dd if=in of=out ibs=$block_size obs=$block_size oflag=direct \
    || skip_ 'neither 512 byte nor 4 KiB aligned O_DIRECT writes succeeded'
fi

truncate -s $((block_size - 1)) short || framework_failure_
truncate -s 8191 m1 || framework_failure_
truncate -s 8193 p1 || framework_failure_

for i in short m1 p1; do
  rm -f out
  dd if=$i of=out ibs=$block_size obs=$block_size \
    iflag=direct oflag=direct || fail=1
done

Exit $fail
