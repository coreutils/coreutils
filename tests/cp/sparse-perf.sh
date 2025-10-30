#!/bin/sh
# ensure that a sparse file is copied efficiently, by default

# Copyright (C) 2021-2025 Free Software Foundation, Inc.

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
cleanup_() { rm -rf "$other_partition_tmpdir"; }
. "$abs_srcdir/tests/other-fs-tmpdir"

# Create a sparse file on another partition to avoid reflinking
# thus exercising more copy logic

other_partition_sparse=$other_partition_tmpdir/k
printf x > $other_partition_sparse || framework_failure_
truncate -s1M $other_partition_sparse || framework_failure_

# cp should not disable anything by default, even for sparse files.  For e.g.
# copy offload is an important performance improvement for sparse files on NFS.
cp --debug $other_partition_sparse k2 >cp.out || fail=1
cmp $other_partition_sparse k2 || fail=1
grep ': avoided' cp.out && { cat cp.out; fail=1; }


# Create a large-non-sparse-but-compressible file
# Ensure we don't avoid copy offload which we saw with
# transparent compression on OpenZFS at least
# (as that triggers our sparse heuristic).
mls='might-look-sparse'
yes | head -n1M > "$mls" || framework_failure_
cp --debug "$mls" "$mls.cp" >cp.out || fail=1
cmp "$mls" "$mls.cp" || fail=1
grep ': avoided' cp.out && { cat cp.out; fail=1; }


# Create a large-but-sparse file on the current partition.
# We disable relinking below, thus verifying SEEK_HOLE support

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
