#!/bin/sh
# Ensure multi-hardlinked files are not lost on case insensitive file systems

# Copyright (C) 2014-2026 Free Software Foundation, Inc.

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
cleanup_() { cd /; umount "$cwd/mnt"; }

for ocase in '-t ext4 -O casefold' '-t hfsplus'; do
  rm -f case.img
  truncate -s100M case.img || framework_failure_
  mkfs $ocase case.img &&
  mkdir mnt &&
  mount case.img mnt &&
  printf '%s\n' "$ocase" > mnt/type  &&
  break
done

test -f mnt/type || skip_ 'failed to create case insensitive file system'

if grep 'ext4' mnt/type; then
  rm -d mnt/type mnt/lost+found || framework_failure_
  chattr +F mnt || skip_ 'failed to create case insensitive file system'
fi

cd mnt
touch foo
ln foo whatever
returns_ 1 mv foo Foo || fail=1
test -r foo || fail=1

Exit $fail
