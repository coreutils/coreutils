#!/bin/sh
# Ensure that tail seeks to the end of a device

# Copyright (C) 2017-2025 Free Software Foundation, Inc.

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
print_ver_ tail

# need write access to local device
# (even though we don't actually write anything)
require_root_
require_local_dir_

get_device_size() {
  BLOCKDEV=blockdev
  $BLOCKDEV -V >/dev/null 2>&1 || BLOCKDEV=/sbin/blockdev
  $BLOCKDEV --getsize64 "$1"
}

# Get path to device the current dir is on.
# Note df can only get fs size, not device size.
device=$(df --output=source . | tail -n1) || framework_failure_

dev_size=$(get_device_size "$device") ||
  skip_ "failed to determine size of $device"

tail_offset=$(expr $dev_size - 1023) ||
  skip_ "failed to determine tail offset"

timeout 10 tail -c 1024 "$device" > end1 || fail=1
timeout 10 tail -c +"$tail_offset" "$device" > end2 || fail=1
test $(wc -c < end1) = 1024 || fail=1
cmp end1 end2 || fail=1

Exit $fail
