#!/bin/sh
# Copyright (C) 2021-2025 Free Software Foundation, Inc.

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
print_ver_ cat

# \r followed by \n is displayed as ^M$
# Up to and including 8.32 the $ would have displayed at the start of the line
# overwriting the first character
printf 'a\rb\r\nc\n\r\nd\r' > 'in' || framework_failure_
printf 'a\rb^M$\nc$\n^M$\nd\r' > 'exp' || framework_failure_
cat -E 'in' > out || fail=1
compare exp out || fail=1

# Ensure \r\n spanning files (or buffers) is handled
printf '1\r' > in2 || framework_failure_
printf '\n2\r\n' > in2b || framework_failure_
printf '1^M$\n2^M$\n' > 'exp' || framework_failure_
cat -E 'in2' 'in2b' > out || fail=1
compare exp out || fail=1

# Ensure \r at end of buffer is handled
printf '1\r' > in2 || framework_failure_
printf '2\r\n' > in2b || framework_failure_
printf '1\r2^M$\n' > 'exp' || framework_failure_
cat -E 'in2' 'in2b' > out || fail=1
compare exp out || fail=1

Exit $fail
