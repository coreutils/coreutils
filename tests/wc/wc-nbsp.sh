#!/bin/sh
# Test non breaking space handling

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
print_ver_ wc printf

# Before coreutils 8.31 nbsp was treated as part of a word,
# rather than a word delimiter

check_word_sep() {
  char="$1"
  # Use -L to determine whether NBSP is printable.
  # FreeBSD 11 and OS X treat NBSP as non printable ?
  if test "$(env printf "=$char=" | wc -L)" = 3; then
    test $(env printf "=$char=" | wc -w) = 2 || fail=1
  fi
}

export LC_ALL=en_US.iso8859-1  # only lowercase form works on macOS 10.15.7
if test "$(locale charmap 2>/dev/null | sed 's/iso/ISO-/')" = ISO-8859-1; then
  check_word_sep '\xA0'
fi

export LC_ALL=en_US.UTF-8
if test "$(locale charmap 2>/dev/null)" = UTF-8; then
  #non breaking space class
  check_word_sep '\u00A0'
  check_word_sep '\u2007'
  check_word_sep '\u202F'
  check_word_sep '\u2060'

  #sampling of "standard" space class
  check_word_sep '\u0020'
  check_word_sep '\u2003'
fi

export LC_ALL=ru_RU.KOI8-R
if test "$(locale charmap 2>/dev/null)" = KOI8-R; then
  check_word_sep '\x9A'
fi

Exit $fail
