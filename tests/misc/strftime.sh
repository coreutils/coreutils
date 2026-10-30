#!/bin/sh
# test strftime edge cases from https://bugs.gnu.org/82130

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
print_ver_ date ls pr
getlimits_

# Check gnulib commit 05812ed5
returns_ 1 date -d @0 "+%${SSIZE_OFLOW}Y" 2>err || fail=1
printf 'date: fprintftime error: %s\n' "$ERANGE" >exp-err || framework_failure_
compare exp-err err || fail=1

# Check gnulib commit 8017c478
ls -ld --time-style='+%504F' . >out || fail=1
test "$(tr -cd '0' < out | wc -c)" -ge 400 || fail=1

# Check gnulib commit 8189e652
ls -ld --time-style='+%_10000z' . || fail=1

# Check gnulib commit 197a7b31
# Note this requires ASAN to trigger with accelerated memset
ls -ld --time-style='+%_1z' . || fail=1
# Note this does not require ASAN to trigger
pr -D'%_1z' /dev/null || fail=1

Exit $fail
