#!/bin/sh
# Test that tac --separator=SEP works if SEP is not ASCII.

# Copyright (C) 2025-2026 Free Software Foundation, Inc.

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
print_ver_ tac printf

check_separator ()
{
  env printf "1$12$13$1" > inp || framework_failure_
  env printf "3$12$11$1\n" > exp || framework_failure_
  tac --separator="$(env printf -- "$1")" inp > out || fail=1
  env printf '\n' >> out || framework_failure_
  compare exp out || fail=1
}

export LC_ALL=en_US.iso8859-1  # only lowercase form works on macOS 10.15.7
if test "$(locale charmap 2>/dev/null | sed 's/iso/ISO-/')" = ISO-8859-1; then
  check_separator '\xe9'  # é
  check_separator '\xe9\xea'  # éê
fi

export LC_ALL=$LOCALE_FR_UTF8
if test "$(locale charmap 2>/dev/null)" = UTF-8; then
  check_separator '\u0434'  # д
  check_separator '\u0434\u0436'  # дж
  # invalid UTF8|unpaired surrogate|C1 control|noncharacter
  check_separator '\xC3|\xED\xBA\xAD|\u0089|\xED\xA6\xBF\xED\xBF\xBF'
fi

Exit $fail
