#!/bin/sh

# Copyright (C) 2026 Free Software Foundation, Inc.

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
print_ver_ whoami logname
skip_if_root_

unshare -U unshare --version || skip_ 'unshare -U is unavailable'
overflow_uid=$(cat /proc/sys/kernel/overflowuid) ||
 skip_ 'overflow uid is unavailable'

test "$(unshare -U whoami)" = "$(id -un $overflow_uid)" || fail=1

# The "</dev/null" disables a fallback lookup via utmp/utmpx,
# that existed in glibc < 2.28 and exists again in glibc >= 2.38.
returns_ 1 unshare -U logname </dev/null 2>err || fail=1
test "$(cat err)" = "logname: no login name" || fail=1

Exit $fail
