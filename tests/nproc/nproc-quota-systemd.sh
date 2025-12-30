#!/bin/sh
# Ensure that "nproc" honors cgroup quotas and kernel scheduler from systemd

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

systemd-run --version || skip_ 'Not a systemd system'

all=$(nproc --all)
test ${all} != 1 || skip_ 'Maybe, 1 thread system?'

test $(systemd-run --scope -q -p CPUQuota=100% chrt --other 0 nproc) = 1 || fail=1
test $(systemd-run --scope -q -p CPUQuota=149% chrt --other 0 nproc) = 1 || fail=1
test $(systemd-run --scope -q -p CPUQuota=150% chrt --other 0 nproc) = 2 || fail=1
test $(systemd-run --scope -q -p CPUQuota=249% chrt --other 0 nproc) = 2 || fail=1

test $(systemd-run --scope -q -p CPUQuota=100% chrt --idle 0 nproc) = 1 || fail=1
test $(systemd-run --scope -q -p CPUQuota=100% chrt --batch 0 nproc) = 1 || fail=1
# some scheduler should use all threads
test $(systemd-run --scope -q -p CPUQuota=100% chrt --fifo 1 nproc) = ${all} || fail=1
test $(systemd-run --scope -q -p CPUQuota=100% chrt --rr 1 nproc) = ${all} || fail=1
test $(systemd-run --scope -q -p CPUQuota=100% chrt --deadline --sched-runtime 100000000 --sched-deadline 1000000000 --sched-period 1000000000 0 nproc) = ${all} || fail=1

Exit $fail
