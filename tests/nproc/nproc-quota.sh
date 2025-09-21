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
require_gcc_shared_

# Replace sched_getscheduler()
cat > k.c <<'EOF' || framework_failure_
#include <stdio.h>
#include <errno.h>
#define __USE_GNU  /* For SCHED_DEADLINE.  */
#include <sched.h>

int
sched_getscheduler (pid_t pid)
{
  fclose (fopen ("preloaded","w")); /* marker for preloaded interception */
  FILE* policyf = fopen ("/proc/self/sched", "r");
  int policy;
  #define fscanfmt fscanf  /* Avoid syntax check.  */
  if (pid == 0 && fscanfmt (policyf, "policy : %d", &policy) == 1)
  {
     switch (policy)
       {
         case 0:  return SCHED_OTHER;
         case 1:  return SCHED_FIFO;
         case 2:  return SCHED_RR;
         case 6:  return SCHED_DEADLINE;
         case -1: errno = EINVAL; return -1;
         default: return SCHED_OTHER;
       }
  }
  else
    {
      errno = ENOSYS;
      return -1;
    }
}
EOF

# compile/link the interception shared library:
gcc_shared_ k.c k.so \
  || skip_ 'failed to build sched_getscheduler shared library'

(export LD_PRELOAD=$LD_PRELOAD:./k.so
 nproc) || fail=1

# Ensure our wrapper is in place and is called
# I.e., this test is restricted to new enough Linux systems
# as otherwise cpu_quota() will not call sched_getscheduler()
test -e preloaded || skip_ 'LD_PRELOAD interception failed'

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
  echo 'policy   :   0' > $ROOT/proc/self/sched &&
  echo 'max 100000' > $ROOT/sys/fs/cgroup/foo/cpu.max  # ignored
} || framework_failure_

nproc=$abs_top_builddir/src/nproc$EXEEXT
cp --parents $(ldd $nproc | grep -o '/[^ ]*') $ROOT ||
  skip_ 'Failed to copy nproc libs to chroot'
cp $nproc $ROOT || framework_failure_
cp k.so $ROOT || framework_failure_

NPROC() { LD_PRELOAD=$LD_PRELOAD:./k.so chroot $ROOT /nproc "$@"; }
NPROC --version ||
  skip_ 'Failed to execute nproc in chroot'

unset OMP_NUM_THREADS
unset OMP_THREAD_LIMIT

ncpus=$(nproc) || fail=1

# Avoid the following block if the system may
# already have CPU quotas set, as our simulation
# would then not match $ncpus
if test "$ncpus" = "$(nproc --all)"; then
echo 'max 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(NPROC) -eq $ncpus || fail=1
# Note Linux won't allow values > 1,000,000 (i.e., micro seconds)
# so no point testing above that threshold ( for e.g $((1<<53)) )
echo "1000000 1" > $ROOT/sys/fs/cgroup/cpu.max &&
test $(NPROC) -eq $ncpus || fail=1
# Can't happen in real cgroup, but ensure we avoid divide by zero
echo '100000 0' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(NPROC) -eq $ncpus || fail=1
echo '100000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(OMP_NUM_THREADS=$ncpus NPROC) -eq $ncpus || fail=1

echo '100000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
echo 'policy   :   1' > $ROOT/proc/self/sched && # No quota for SCHED_FIFO
test $(NPROC) -eq $ncpus || fail=1
echo 'policy   :   2' > $ROOT/proc/self/sched && # No quota for SCHED_RR
test $(NPROC) -eq $ncpus || fail=1
echo 'policy   :   6' > $ROOT/proc/self/sched && # No quota for SCHED_DEADLINE
test $(NPROC) -eq $ncpus || fail=1
echo 'policy   :  -1' > $ROOT/proc/self/sched && # Unsupported
test $(NPROC) -eq $ncpus || fail=1
echo 'policy   :   0' > $ROOT/proc/self/sched # reset to SCHED_OTHER
fi

echo '40000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(NPROC) -eq 1 || fail=1
echo '50000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(NPROC) -eq 1 || fail=1
echo '100000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(NPROC) -eq 1 || fail=1
echo '140000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(NPROC) -eq 1 || fail=1
echo '1 1000000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(NPROC) -eq 1 || fail=1

if test $ncpus -gt 1; then
echo '150000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(NPROC) -eq 2 || fail=1
echo '150000 100000' > $ROOT/sys/fs/cgroup/cpu.max &&
test $(OMP_THREAD_LIMIT=10 NPROC) -eq 2 || fail=1
fi

Exit $fail
