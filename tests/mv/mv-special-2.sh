#!/bin/sh
# Ensure that mv works with non standard copies across file systems

# Copyright (C) 2025 Free Software Foundation, Inc.

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
print_ver_ mv

require_root_

cwd=$(pwd)
cleanup_() { cd /; umount "$cwd/xdev"; }

skip=0

# Mount an ext2 loopback file system at $WHERE
make_fs() {
  where="$1"

  mkdir "$where"  || framework_failure_

  fs="$where.bin"
  dd if=/dev/zero of="$fs" bs=8192 count=200 > /dev/null 2>&1 || skip=1
  mkfs -t ext2 -F "$fs" || skip_ "failed to create ext2 file system"
  mount -oloop "$fs" "$where" || skip=1

  echo test > "$where"/f && test -s "$where"/f || skip=1

  test $skip = 1 && skip_ 'insufficient mount/ext2 support'
}

make_fs xdev

truncate -s 8G huge || framework_failure_
mv --verbose huge xdev &&
returns_ 1 test -e huge &&
test -s xdev/huge || fail=1

mknod devzero c 1 5 || framework_failure_
mv --verbose devzero xdev &&
returns_ 1 test -c devzero &&
test -c xdev/devzero || fail=1

ln -nsf blah blah   || framework_failure_
mv --verbose blah xdev &&
returns_ 1 test -L blah &&
test -L xdev/blah || fail=1

if python -c "import socket as s; s.socket(s.AF_UNIX).bind('test.sock')" &&
   test -S 'test.sock'; then
  mv --verbose test.sock xdev &&
  returns_ 1 test -S test.sock &&
  test -S xdev/test.sock || fail=1
fi

Exit $fail
