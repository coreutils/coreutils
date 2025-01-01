#!/bin/sh
# Ensure symlinks can be replaced across devices

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
print_ver_ cp
require_root_

cwd=$(pwd)
cleanup_() { cd /; umount "$cwd/mnt"; }

truncate -s100M fs.img || framework_failure_
mkfs -t ext4 fs.img    || skip_ 'failed to create ext4 file system'
mkdir mnt              || framework_failure_
mount fs.img mnt       || skip_ 'failed to mount ext4 file system'

mkdir mnt/path1 || framework_failure_
touch mnt/path1/file || framework_failure_
mkdir path2 || framework_failure_
cd path2 && ln -s ../mnt/path1/file || framework_failure_

cp -dsf ../mnt/path1/file . || fail=1

Exit $fail
