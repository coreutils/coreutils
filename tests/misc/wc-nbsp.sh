#!/bin/sh
# Test non breaking space handling

# Copyright (C) 2019 Free Software Foundation, Inc.

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
print_ver_ wc printf

# Before coreutils 8.31 nbsp was treated as part of a word,
# rather than a word delimiter

export LC_ALL=en_US.ISO-8859-1
if test "$(locale charmap 2>/dev/null)" = ISO-8859-1; then
  test $(env printf '=\xA0=' | wc -w) = 2 || fail=1
  test $(env printf '=\xA0=' | POSIXLY_CORRECT=1 wc -w) = 1 || fail=1
fi
export LC_ALL=en_US.UTF-8
if test "$(locale charmap 2>/dev/null)" = UTF-8; then
  test $(env printf '=\u00A0=' | wc -w) = 2 || fail=1
  test $(env printf '=\u2007=' | wc -w) = 2 || fail=1
  test $(env printf '=\u202F=' | wc -w) = 2 || fail=1
  test $(env printf '=\u2060=' | wc -w) = 2 || fail=1
fi
export LC_ALL=ru_RU.KOI8-R
if test "$(locale charmap 2>/dev/null)" = KOI8-R; then
  test $(env printf '=\x9A=' | wc -w) = 2 || fail=1
fi

Exit $fail
