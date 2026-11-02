#!/bin/sh
# Test for output with appropriate precision

# Copyright (C) 2019-2026 Free Software Foundation, Inc.

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
print_ver_ seq printf

# With coreutils-8.30 and earlier, the last decimal point would be ','
# when setlocale(LC_ALL, "") failed, but setlocale(LC_NUMERIC, "") succeeded.
LC_ALL= LANG=invalid LC_NUMERIC=$LOCALE_FR_UTF8 seq 0.1 0.2 0.7 > out || fail=1
uniq -w2 out > out-merge || framework_failure_
test "$(wc -l < out-merge)" = 1 || { fail=1; cat out; }

# With coreutils-9.12 and earlier, operands using the locale's decimal point
# were treated as integers, and so a step less than 1 output FIRST indefinitely.
if test "$LOCALE_FR_UTF8" != none &&
  test "$(LC_ALL=$LOCALE_FR_UTF8 env printf '%.1f' 0,5)" = '0,5'; then

  printf '%s\n' 0,0 0,5 1,0 > exp || framework_failure_
  LC_ALL=$LOCALE_FR_UTF8 seq 0 0,5 1 | head -n4 > out || fail=1
  compare exp out || fail=1

  printf '%s\n' 1,5 2,5 > exp || framework_failure_
  LC_ALL=$LOCALE_FR_UTF8 seq 1,5 3 > out || fail=1
  compare exp out || fail=1

  printf '%s\n' -0,5 00,5 > exp || framework_failure_
  LC_ALL=$LOCALE_FR_UTF8 seq -w -,5 ,5 > out || fail=1
  compare exp out || fail=1
fi

# Likewise for hex floats with an upper case exponent character.
printf '%s\n' 0 0.5 1 > exp || framework_failure_
seq 0 0x1P-1 1 | head -n4 > out || fail=1
compare exp out || fail=1

Exit $fail
