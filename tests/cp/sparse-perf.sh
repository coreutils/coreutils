#!/bin/sh
# ensure that a sparse file is copied efficiently, by default

# Copyright (C) 2021-2023 Free Software Foundation, Inc.

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

# Create a large-but-sparse file.
timeout 10 truncate -s1T f ||
  skip_ "unable to create a 1 TiB sparse file"

# Note zfs with zfs_dmu_offset_next_sync=0 (the default)
# will generally skip here, due to needing about 5 seconds
# between the creation of the file and the use of SEEK_DATA,
# for it to determine it's an empty file (return ENXIO).
seek_data_capable_ f ||
  skip_ "insufficient SEEK_DATA support"

# Nothing can read that many bytes in so little time.
timeout 10 cp --reflink=never f f2 || fail=1

# Ensure that the sparse file copied through SEEK_DATA has the same size
# in bytes as the original.
test "$(stat --printf %s f)" = "$(stat --printf %s f2)" || fail=1

Exit $fail
