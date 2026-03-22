#!/bin/sh
# Test that 'pwd' works when given an argument.

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
print_ver_ pwd
getlimits_

base=$(env pwd)
mkdir -p a/b/c || framework_failure_
cd a/b/c || framework_failure_

echo 'pwd: ignoring non-option arguments' > exp-err
echo "$base/a/b/c" > exp-out
env pwd a > out 2> err || fail=1
compare exp-out out || fail=1
compare exp-err err || fail=1

Exit $fail
