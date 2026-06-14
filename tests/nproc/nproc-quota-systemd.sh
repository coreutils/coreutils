#!/bin/sh
# Ensure that "nproc" honors cgroup quotas and kernel scheduler from systemd

# Copyright (C) 2025-2026 Free Software Foundation, Inc.

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

systemd-run --version || skip_ 'systemd-run not available'

all=$(nproc --all)
test ${all} != 1 || skip_ 'Single CPU detected'

check_sched_quota()
{
  cpu=$1; shift
  sysd_limit=$1; shift

  if systemd-run --scope -q -p $sysd_limit chrt $* true; then
    test $(systemd-run --scope -q -p $sysd_limit \
           chrt $* \
           env OMP_NUM_THREADS=0 \
           nproc) = $cpu ||
     return 1
  fi
  return 0
}

check_sched_quota 1 CPUQuota=100% --other 0 || fail=1
check_sched_quota 1 CPUQuota=149% --other 0 || fail=1
check_sched_quota 2 CPUQuota=150% --other 0 || fail=1
check_sched_quota 2 CPUQuota=249% --other 0 || fail=1

check_sched_quota 1 CPUQuota=100% --idle 0 || fail=1
check_sched_quota 1 CPUQuota=100% --batch 0 || fail=1

# some schedulers should use all threads
check_sched_quota $all CPUQuota=100% --fifo 1 || fail=1
check_sched_quota $all CPUQuota=100% --rr 1 || fail=1
check_sched_quota $all CPUQuota=100% --deadline \
 --sched-runtime 100000000 --sched-deadline 1000000000 \
 --sched-period 1000000000 0 || fail=1

# Ensure affinity mask is applied
check_sched_quota 1 AllowedCPUs=0 --other 0 || fail=1
check_sched_quota 1 AllowedCPUs=0 --fifo 1 || fail=1
check_sched_quota 1 AllowedCPUs=0 --rr 1 || fail=1

Exit $fail
