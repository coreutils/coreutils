#!/bin/sh
# Test 'uniq -c' with various integer widths.

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
print_ver_ uniq
expensive_

count=$(shuf -i 10000000-99999999 -n 1) || framework_failure_

while test $count -gt 0; do
  printf '%7d y\n' $count >exp || framework_failure_
  yes | head -n $count | uniq -c >out 2>err || fail=1
  compare exp out || fail=1
  compare /dev/null err || fail=1
  count=$((count / 10))
done

Exit $fail
