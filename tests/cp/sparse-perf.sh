#!/bin/sh
# ensure that a sparse file is copied efficiently, by default

# Copyright (C) 2011-2021 Free Software Foundation, Inc.

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

touch sparse_chk
seek_data_capable_ sparse_chk ||
  skip_ "this file system lacks SEEK_DATA support"

# Create a large-but-sparse file.
timeout 10 truncate -s1T f ||
  skip_ "unable to create a 1 TiB sparse file"

# Nothing can read (much less write) that many bytes in so little time.
timeout 10 cp --reflink=never f f2
ret=$?
if test $ret -eq 124; then  # timeout
  # Only fail if we allocated more data
  # as we've seen SEEK_DATA taking 35s on some freebsd VMs
  test $(stat -c%b f2) -gt $(stat -c%b f) && fail=1 ||
    skip_ "SEEK_DATA timed out"
elif test $ret -ne 0; then
  fail=1
fi

# Ensure that the sparse file copied through SEEK_DATA has the same size
# in bytes as the original.
test "$(stat --printf %s f)" = "$(stat --printf %s f2)" || fail=1

Exit $fail
