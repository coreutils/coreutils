#!/bin/sh
# Copyright (C) 2005-2026 Free Software Foundation, Inc.

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

# Test for the 2005-10-13 patch to lib/mkdir-p.c that fixed this sort
# of bug in mkdir:
#
#   "mkdir -p /a/b/c" no longer fails merely because a leading prefix
#   directory (e.g., /a or /a/b) exists on a read-only file system.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ mkdir
require_root_

cwd=$(pwd)
cleanup_() { cd /; umount "$cwd/mnt-ro/rw"; umount "$cwd/mnt-ro"; }

dd if=/dev/zero of=1 bs=8192 count=50 &&
dd if=/dev/zero of=2 bs=8192 count=50 &&
mkdir -p mnt-ro && mkfs -t ext2 1 && mkfs -t ext2 2 &&
mount -o loop 1 mnt-ro &&
mkdir -p mnt-ro/rw &&
mount -o remount,ro mnt-ro &&
mount -o loop 2 mnt-ro/rw ||
skip_ 'Failed to setup loopback mounts'

mkdir -p mnt-ro/rw/sub || fail=1

Exit $fail
