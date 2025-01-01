#!/bin/sh
# Test join in a UTF-8 locale.

# Copyright 2023-2025 Free Software Foundation, Inc.

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
print_ver_ join

test "$LOCALE_FR_UTF8" != none || skip_ "French UTF-8 locale not available"

LC_ALL=$LOCALE_FR_UTF8
export LC_ALL

vertical_line='|'
multiplication_sign='×'
en_dash='–'
old_Persian_word_divider='𐏐'

tflag=

for s in \
    ' ' \
    "$vertical_line" \
    "$multiplication_sign" \
    "$en_dash" \
    "$old_Persian_word_divider"
do
  printf '0%sA\n1%sa\n2%sb\n4%sc\n' "$s" "$s" "$s" "$s" >a ||
    framework_failure_
  printf '0%sB\n1%sd\n3%se\n4%s\0f\n' "$s" "$s" "$s" "$s" >b ||
    framework_failure_
  join $tflag$s -a1 -a2 -eouch -o0,1.2,2.2 a b >out || fail=1
  tflag=-t
  printf '0%sA%sB\n1%sa%sd\n2%sb%souch\n3%souch%se\n4%sc%s\0f\n' \
         "$s" "$s" "$s" "$s" "$s" "$s" "$s" "$s" "$s" "$s" >exp ||
    framework_failure_
  compare exp out || fail=1
done

Exit $fail
