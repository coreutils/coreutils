#!/bin/sh
# Validate cksum operation

# Copyright (C) 2020-2021 Free Software Foundation, Inc.

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
print_ver_ cksum printf

returns_ 1 cksum missing || fail=1

{
  for offset in $(seq -1 6); do
    env printf $(env printf '\\%03o' $(seq 0 $offset));
    env printf $(env printf '\\%03o' $(seq 0 255));
  done
} > in || framework_failure_

cksum in > out || fail=1
printf '%s\n' '4097727897 2077 in' > exp || framework_failure_
compare exp out || fail=1

Exit $fail
