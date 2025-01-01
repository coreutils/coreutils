#!/bin/sh
# Ensure that df outputs one line per entry

# Copyright (C) 2012-2025 Free Software Foundation, Inc.

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
print_ver_ df printf
require_root_


# Ensure a new line in a mount point only outputs a single line

mnt='mount
point'

cwd=$(pwd)
cleanup_() { umount "$cwd/$mnt"; }

skip=0
# Create a file system, then mount it.
dd if=/dev/zero of=blob bs=8192 count=200 > /dev/null 2>&1 \
                                             || skip=1
mkdir "$mnt"                                 || skip=1
mkfs -t ext2 -F blob \
  || skip_ "failed to create ext2 file system"
mount -oloop blob "$mnt"                     || skip=1
test $skip = 1 \
  && skip_ "insufficient mount/ext2 support"
test $(df "$mnt" | wc -l) = 2 || fail=1
test "$fail" = 1 && dump_mount_list_


# Ensure mount points not matching the current user encoding are output

unset LC_ALL
if test "$LOCALE_FR_UTF8" != "none"; then
  f=$LOCALE_FR_UTF8

  cleanup_ || framework_failure_

  # Create a non UTF8 mount point
  mnt="$(env printf 'm\xf3unt p\xf3int')"
  mkdir "$mnt" || framework_failure_
  mount -oloop blob "$mnt" || skip_ "unable to mount $mnt"

  LC_ALL=$f df --output=target "$mnt" > df.out || fail=1
  test "$(basename "$(tail -n1 df.out)")" = "$mnt" || fail=1
fi


Exit $fail
