#!/bin/sh
# Verify behavior of ls --zero.

# Copyright 2021 Free Software Foundation, Inc.

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

mkdir dir && touch dir/a dir/b dir/cc || framework_failure_

LC_ALL=C ls --zero dir >out || fail=1
tr '\n' '\0' <<EOF >exp
a
b
cc
EOF

compare exp out || fail=1

Exit $fail
