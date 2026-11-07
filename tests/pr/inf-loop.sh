#!/bin/sh
# Test a case that would make 'pr' infinite loop in coreutils-9.12 and earlier.

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
print_ver_ pr

# Test that we don't have an infinite loop when the input line length is longer
# than the page width.
yes "$(printf '\t')" | head -n 100 >inp &&
tr -d '\t' <inp >exp || framework_failure_
timeout 10 pr -t -W 7 inp >out 2>err || fail=1
compare exp out || fail=1
compare /dev/null err || fail=1

# Make sure we don't lose line numbers.
for i in $(seq 100); do printf '%5d\t\n' "$i" || framework_failure_; done >exp
timeout 10 pr -n -t -W 7 inp >out 2>err || fail=1
compare exp out || fail=1
compare /dev/null err || fail=1

Exit $fail
