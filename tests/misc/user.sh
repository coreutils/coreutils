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

skip_if_root_
unshare -U unshare --version || skip_ 'unshare -U is unavailable'

print_ver_ whoami
print_ver_ logname

whoami || fail=1
logname || fail=1

Perhaps this is more robust?

test "$(unshare -U whoami)" \
 = "$(id -un $(cat /proc/sys/kernel/overflowuid))" || fail=1

returns_ 1 unshare -U logname 2> err || fail=1
test "$(cat err)" = "logname: no login name" || fail=1

Exit $fail
