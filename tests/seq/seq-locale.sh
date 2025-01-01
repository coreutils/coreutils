#!/bin/sh
# Test for output with appropriate precision

# Copyright (C) 2019-2025 Free Software Foundation, Inc.

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

# With coreutils-8.30 and earlier, the last decimal point would be ','
# when setlocale(LC_ALL, "") failed, but setlocale(LC_NUMERIC, "") succeeded.
LC_ALL= LANG=invalid LC_NUMERIC=$LOCALE_FR_UTF8 seq 0.1 0.2 0.7 > out || fail=1
uniq -w2 out > out-merge || framework_failure_
test "$(wc -l < out-merge)" = 1 || { fail=1; cat out; }

Exit $fail
