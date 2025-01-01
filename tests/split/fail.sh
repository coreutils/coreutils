#!/bin/sh
# split must fail when given length/count of zero.

# Copyright (C) 2003-2025 Free Software Foundation, Inc.

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
print_ver_ split
getlimits_

touch in || framework_failure_


split -a 0 in 2> /dev/null || fail=1
returns_ 1 split -b 0 in 2> /dev/null || fail=1
returns_ 1 split -C 0 in 2> /dev/null || fail=1
returns_ 1 split -l 0 in 2> /dev/null || fail=1
returns_ 1 split -n 0 in 2> /dev/null || fail=1
returns_ 1 split -n 1/0 in 2> /dev/null || fail=1
returns_ 1 split -n 0/1 in 2> /dev/null || fail=1
returns_ 1 split -n 2/1 in 2> /dev/null || fail=1

# Make sure -C doesn't create empty files.
rm -f x?? || fail=1
echo x | split -C 1 || fail=1
test -f xaa && test -f xab || fail=1
test -f xac && fail=1

# Make sure that the obsolete -N notation still works
split -1 in 2> /dev/null || fail=1

# Then make sure that -0 evokes a failure.
returns_ 1 split -0 in 2> /dev/null || fail=1

split --lines=$UINTMAX_MAX in || fail=1
split --bytes=$OFF_T_MAX in || fail=1
split --line-bytes=$OFF_T_OFLOW in || fail=1
split --line-bytes=$SIZE_OFLOW in || fail=1
if truncate -s$SIZE_OFLOW large; then
  # Ensure we can split chunks of a large file on 32 bit hosts
  split --number=$SIZE_OFLOW/$SIZE_OFLOW large >/dev/null || fail=1
fi
split --number=r/$INTMAX_MAX/$UINTMAX_MAX </dev/null >/dev/null || fail=1
returns_ 1 split --number=r/$UINTMAX_OFLOW </dev/null 2>/dev/null || fail=1

# Make sure that a huge obsolete option does the right thing.
split -99999999999999999991 in || fail=1

Exit $fail
