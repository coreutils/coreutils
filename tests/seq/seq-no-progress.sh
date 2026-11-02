#!/bin/sh
# Ensure seq terminates when floating point arithmetic stops progressing.

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
print_ver_ seq

# 2**130 is exactly representable in a 64 bit double
big=1361129467683753853853498429727072845824
test $(timeout 10 seq -f '%.0f' "$big" "$big" "$big") = "$big" ||
  skip_ 'seq not built with double support'

check_seq_ ()
{
  printf '%s\n' "$1" >exp || framework_failure_
  shift

  { timeout 10 seq "$@" 2>err; echo $? >status; } | head -n2 >out
  echo 0 >exp-status || framework_failure_
  compare exp-status status || fail=1
  compare /dev/null err || fail=1
  compare exp out || fail=1
}

check_seq_ "$big" -w "$big" "$big"
check_seq_ "$big" "$big" 201 "$big"
check_seq_ "-$big" -- "-$big" "-$big"
check_seq_ "-$big" -- "-$big" -1 "-$big"

check_seq_ 1.0000000000000000000000000000000000000000 1 1e-40 1
check_seq_ -inf -- -inf -inf
# Adding opposite infinities produces NaN, which must terminate the loop.
check_seq_ -inf -- -inf inf 0

Exit $fail
