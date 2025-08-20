#!/bin/sh
# Ensure that "nproc" honors cgroup quotas

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
print_ver_ nproc
require_root_

# We could look for modifiable cgroup quotas on the current system,
# but that would be dangerous to modify and restore robustly.
# Instead simulate the queried cgroup files in a chroot.
# For recipes for testing with real cgroups see
# the comment in: gnulib/tests/test-nproc.c

ROOT=cgroup
{
  mkdir -p $ROOT/proc/self &&
  mkdir -p $ROOT/sys/fs/cgroup/foo &&
  touch $ROOT/sys/fs/cgroup/cgroup.controllers &&
  echo '0::/foo' > $ROOT/proc/self/cgroup &&
  echo 'max 100000' > $ROOT/sys/fs/cgroup/foo/cpu.max  # ignored
} || framework_failure_

nproc=$abs_top_builddir/src/nproc$EXEEXT
cp --parents $(ldd $nproc | grep -o '/[^ ]*') $ROOT ||
  skip_ 'Failed to copy nproc libs to chroot'
cp $nproc $ROOT || framework_failure_
chroot $ROOT /nproc --version ||
  skip_ 'Failed to execute nproc in chroot'

unset OMP_NUM_THEADS
unset OMP_THREAD_LIMIT

ncpus=$(nproc) || fail=1

# Avoid the following block if the system may
# already have CPU quotas set, as our simulation
# would then not match $ncpus
if test "$ncpus" = "$(nproc --all)"; then
echo 'max 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(chroot $ROOT /nproc) -eq $ncpus || fail=1
# Note Linux won't allow values > 1,000,000 (i.e., micro seconds)
# so no point testing above that threshold ( for e.g $((1<<53)) )
echo "1000000 1" > $ROOT/sys/fs/cgroup/cpu.max &&
test $(chroot $ROOT /nproc) -eq $ncpus || fail=1
# Can't happen in real cgroup, but ensure we avoid divide by zero
echo '100000 0' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(chroot $ROOT /nproc) -eq $ncpus || fail=1
echo '100000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(OMP_NUM_THREADS=$ncpus chroot $ROOT /nproc) -eq $ncpus || fail=1
fi

echo '40000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(chroot $ROOT /nproc) -eq 1 || fail=1
echo '50000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(chroot $ROOT /nproc) -eq 1 || fail=1
echo '100000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(chroot $ROOT /nproc) -eq 1 || fail=1
echo '140000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(chroot $ROOT /nproc) -eq 1 || fail=1
echo '1 1000000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(chroot $ROOT /nproc) -eq 1 || fail=1

if test $ncpus -gt 1; then
echo '150000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(chroot $ROOT /nproc) -eq 2 || fail=1
echo '150000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(OMP_THREAD_LIMIT=10 chroot $ROOT /nproc) -eq 2 || fail=1
fi

Exit $fail
