#!/bin/sh
# Test paste with standard input closed.

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
print_ver_ paste

echo a > in || framework_failure_
paste in <&- > out 2> err || fail=1
compare in out || fail=1
compare /dev/null err || fail=1

# Diagnose '-' before writing any columns from named files.
echo 'paste: standard input is closed' > exp || framework_failure_
for files in 'in -' '- in'; do
  returns_ 1 paste $files <&- > out 2> err || fail=1
  compare /dev/null out || fail=1
  compare exp err || fail=1
done

# Opening 'in' must not make it available through /dev/stdin.
if test "$(cat /dev/stdin < in 2>/dev/null)" = a &&
   returns_ 1 paste /dev/stdin <&- > /dev/null 2> exp; then
  returns_ 1 paste in /dev/stdin <&- > out 2> err || fail=1
  compare /dev/null out || fail=1
  compare exp err || fail=1
fi

Exit $fail
