#!/bin/sh
# test --group-directories-first

# Copyright (C) 2018-2025 Free Software Foundation, Inc.

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
print_ver_ ls

# Isolate output files from directory being listed
mkdir dir dir/b || framework_failure_
touch dir/a || framework_failure_
ln -s b dir/bl || framework_failure_

ls --group dir > out || fail=1
cat <<\EOF > exp
b
bl
a
EOF
compare exp out || fail=1

ls --group -d dir/* > out || fail=1
cat <<\EOF > exp
dir/b
dir/bl
dir/a
EOF
compare exp out || fail=1

Exit $fail
