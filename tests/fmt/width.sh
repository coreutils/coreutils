#!/bin/sh
# Exercise the fmt -w option.

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
print_ver_ fmt

# Ensure width is max display width.
# Before v9.10, width incorrectly included the \n character
printf 'aa bb cc dd ee' | fmt -w 8 > out || fail=1
cat <<\_EOF_ > exp || framework_failure_
aa bb cc
dd ee
_EOF_
compare exp out || fail=1

printf 'aa bb cc dd ee' | fmt -w 7 > out || fail=1
cat <<\_EOF_ > exp || framework_failure_
aa
bb cc
dd ee
_EOF_
compare exp out || fail=1


Exit $fail
