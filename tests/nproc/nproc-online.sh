#!/bin/sh
# Test behaviour with offline cores

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
uses_strace_

test $(cat /sys/devices/system/cpu/cpu1/online) = 1 || skip_ "cpu1 is offline, or missing"
nproc --all > all || framework_failure_
strace -o /dev/null -e inject=sched_getaffinity:error=ENOSYS nproc > no_affinity || skip_ "no sched_getaffinity support"
# make 1 core offline
cleanup_() { echo 1 > tee /sys/devices/system/cpu/cpu1/online; }
echo 0 > /sys/devices/system/cpu/cpu1/online || framework_failure_

# offline core should be shown
test $(nproc --all) = $(cat all) || fail=1
# do not show offline core
test $(strace -o /dev/null -e inject=sched_getaffinity:error=ENOSYS nproc) != $(cat no_affinity) || fail=1

Exit $fail
