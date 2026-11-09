#!/bin/sh
# Ensure a multi-byte character split across read() calls
# is not processed as separate bytes.

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
print_ver_ cut fold expand unexpand

{ test -z "$LOCALE_FR_UTF8" || test "$LOCALE_FR_UTF8" = none; } &&
  skip_ 'French UTF-8 locale not available'
export LC_ALL="$LOCALE_FR_UTF8"

# Write each byte of U+20AC separately, so that each read() returns one.
euro_in_pieces ()
{
  printf '\342'; sleep .1; printf '\202'; sleep .1; printf '\254'; printf "$1"
}

printf '\342\202\254ab\n' > exp || framework_failure_
euro_in_pieces 'ab\n' | cut -c1-3 > out || fail=1
compare exp out || fail=1

printf '\n' > exp || framework_failure_
euro_in_pieces 'ab\n' | cut -b1-2 -n > out || fail=1
compare exp out || fail=1

printf '\342\202\254a\nb\n' > exp || framework_failure_
euro_in_pieces 'ab\n' | fold -w2 > out || fail=1
compare exp out || fail=1

printf '\342\202\254       b\n' > exp || framework_failure_
euro_in_pieces '\tb\n' | expand > out || fail=1
compare exp out || fail=1

printf '\342\202\254\tb\n' > exp || framework_failure_
euro_in_pieces '       b\n' | unexpand -a > out || fail=1
compare exp out || fail=1

Exit $fail
