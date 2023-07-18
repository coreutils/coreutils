#!/bin/sh
# test splitting into newline delineated chunks from infinite imput

# Copyright (C) 2023 Free Software Foundation, Inc.

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
require_root_

cwd=$(pwd)
cleanup_() { cd /; umount "$cwd/mnt"; }

# Create a file system to provide an isolated $TMPDIR
dd if=/dev/zero of=blob bs=8192 count=200 &&
mkdir mnt                                 &&
mkfs -t ext2 -F blob                      &&
mount -oloop blob mnt                     ||
  skip_ "insufficient mount/ext2 support"
export TMPDIR="$cwd/mnt"

# 'split' should fail eventially when
# creating an infinitely long output file.

returns_ 1 split -n l/2 /dev/zero || fail=1
rm x??

# Repeat the above,  but with 1/2, not l/2:
returns_ 1 split -n 1/2 /dev/zero || fail=1
rm x??

Exit $fail
