#!/bin/sh
# Verify behavior of separate coreutils multicall binary

# Copyright (C) 2014-2026 Free Software Foundation, Inc.

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
print_ver_ coreutils

cp -s "$(command -v coreutils)" blah || skip_ 'multicall binary is disabled'

# Yes outputs all its params so is good to verify argv manipulations
echo 'y' > exp &&
coreutils --coreutils-prog=yes | head -n10 | uniq > out || framework_failure_
compare exp out || fail=1

# Ensure if incorrect program passed, we diagnose
echo "coreutils: unknown program 'blah'" > exp || framework_failure_

returns_ 1 coreutils --coreutils-prog='blah' --help 2>err || fail=1
compare exp err || fail=1

returns_ 1 ./blah 2>err || fail=1
compare exp err || fail=1
returns_ 1 ./blah --version 2>err || fail=1
compare exp err || fail=1

Exit $fail
