#!/bin/sh
# exercise the NaN-infloop bug in coreutils-8.13

# Copyright (C) 2011-2025 Free Software Foundation, Inc.

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
print_ver_ sort

echo nan > F || framework_failure_
printf 'nan\nnan\n' > exp || framework_failure_
timeout 10 sort -g -m F F > out || fail=1

# This was seen to infloop on some systems until coreutils v9.2 (bug 55212)
yes nan | head -n128095 | timeout 60 sort -g > /dev/null || fail=1

compare exp out || fail=1

Exit $fail
