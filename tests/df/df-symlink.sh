#!/bin/sh
# Ensure that df dereferences symlinks to file system nodes

# Copyright (C) 2013-2025 Free Software Foundation, Inc.

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
print_ver_ df

file_system=$(df --out=source '.' | tail -n1) ||
  skip_ "cannot determine '.' file system"

ln -s "$file_system" symlink || framework_failure_

df --out=source,target "$file_system" > exp ||
  skip_ "cannot get info for $file_system"
df --out=source,target symlink > out || fail=1
compare exp out || fail=1

# Ensure we output the same values for device nodes and '.'
# This was not the case in coreutils-8.22 on systems
# where the device in the mount list was a symlink itself.
# I.e., '.' => /dev/mapper/fedora-home -> /dev/dm-2
# Restrict this test to systems with a 1:1 mapping between
# source and target.  This excludes for example BTRFS sub-volumes.
if test "$(df --output=source | grep -F "$file_system" | wc -l)" = 1; then
  # Restrict to systems with a single file system root (and have findmnt(1))
  if test "$(findmnt -nro FSROOT | uniq | wc -l)" =  1; then
    df --out=source,target '.' > out || fail=1
    compare exp out || fail=1
  fi
fi

test "$fail" = 1 && dump_mount_list_

Exit $fail
