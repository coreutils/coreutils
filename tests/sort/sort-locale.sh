#!/bin/sh
# Test sort locale ordering

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
print_ver_ sort ls

check_different_from_C() {
  test "$(printf '%s\n' "$1" "$2" | LC_ALL=C sort)" != \
       "$(printf '%s\n' "$1" "$2" | sort)"
}

check_hard_collate() {
  # Correlate with ls
  touch "$1" "$2" || framework_failure_
  test "$(printf '%s\n' "$1" "$2" | sort)" = "$(ls -1 "$1" "$2")" || fail=1

  if test "$fail" != 1; then
    check_different_from_C "$1" "$2" ||
      skip_ "Strings '$1' '$2' sort the same in C and $LC_ALL locales"
  fi
}

export LC_ALL=en_US.iso8859-1  # only lowercase form works on macOS 10.15.7
if test "$(locale charmap 2>/dev/null | sed 's/iso/ISO-/')" = ISO-8859-1; then
  check_hard_collate 'a_a' 'a b'  # underscore and space considered equal
  check_hard_collate 'aaa' 'BBB'  # case insensitive ordering
  check_hard_collate "$(printf 'aa\351')" 'aaf'  # é comes before f
fi

export LC_ALL=$LOCALE_FR_UTF8
if test "$(locale charmap 2>/dev/null)" = UTF-8; then
  check_hard_collate 'aaé' 'aaf'  # é comes before f
  check_hard_collate 'aéY' "$(printf 'ae\314\201Z')"  # NFC/NFD é are equal
fi

Exit $fail
