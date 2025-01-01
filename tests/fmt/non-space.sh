#!/bin/sh
# Test fmt space handling

# Copyright (C) 2022-2025 Free Software Foundation, Inc.

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
print_ver_ fmt printf

# Before coreutils 9.1 macOS treated bytes like 0x85
# as space characters in multi-byte locales (including UTF-8)

check_non_space() {
  char="$1"
  test "$(env printf "=$char=" | fmt -s -w1 | wc -l)" = 1 || fail=1
}

export LC_ALL=en_US.iso8859-1  # only lowercase form works on macOS 10.15.7
if test "$(locale charmap 2>/dev/null | sed 's/iso/ISO-/')" = ISO-8859-1; then
  check_non_space '\xA0'
fi

export LC_ALL=en_US.UTF-8
if test "$(locale charmap 2>/dev/null)" = UTF-8; then
  check_non_space '\u00A0'  # No break space
  check_non_space '\u2007'  # TODO: should probably split on figure space
  check_non_space '\u202F'  # Narrow no break space
  check_non_space '\u2060'  # zero-width no break space
  check_non_space '\u0445'  # Cyrillic kha, for which macOS isspace(0x85)==true
fi

export LC_ALL=ru_RU.KOI8-R
if test "$(locale charmap 2>/dev/null)" = KOI8-R; then
  check_non_space '\x9A'
fi

Exit $fail
