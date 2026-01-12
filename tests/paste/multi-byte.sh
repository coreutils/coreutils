#!/bin/sh
# Test multi-byte delimiter handling in paste

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
print_ver_ paste printf

test "$LOCALE_FR_UTF8" != none || skip_ 'French UTF-8 locale not available'

LC_ALL=$LOCALE_FR_UTF8
export LC_ALL

# UTF-8 test: 2-byte character (e.g., cent sign)
delim_cent=$(env printf '\xc2\xa2')
# UTF-8 test: 3-byte character (e.g., euro sign)
delim_euro=$(env printf '\xe2\x82\xac')
# UTF-8 test: 4-byte character (e.g., emoji: U+1F600)
delim_emoji=$(env printf '\xf0\x9f\x98\x80')

printf '1\n2\n' > f1 || framework_failure_
printf 'a\nb\n' > f2 || framework_failure_

# Test parallel mode with multi-byte delimiters
for delim in "$delim_cent" "$delim_euro" "$delim_emoji"; do
  paste -d "$delim" f1 f2 > out || fail=1
  printf "1${delim}a\n2${delim}b\n" > exp || framework_failure_
  compare exp out || fail=1
done

# Test serial mode with multi-byte delimiters
printf '1\n2\n3\n' > f3 || framework_failure_
for delim in "$delim_cent" "$delim_euro"; do
  paste -s -d "$delim" f3 > out || fail=1
  printf "1${delim}2${delim}3\n" > exp || framework_failure_
  compare exp out || fail=1
done

# Test multiple multi-byte delimiters cycling
printf 'a\nb\nc\n' > f4 || framework_failure_
printf '1\n2\n3\n' > f5 || framework_failure_
printf 'x\ny\nz\n' > f6 || framework_failure_
paste -d "${delim_cent}${delim_euro}" f4 f5 f6 > out || fail=1
printf "a${delim_cent}1${delim_euro}x\n" > exp || framework_failure_
printf "b${delim_cent}2${delim_euro}y\n" >> exp || framework_failure_
printf "c${delim_cent}3${delim_euro}z\n" >> exp || framework_failure_
compare exp out || fail=1

# Test multi-byte delimiters mixed with empty delimiter (\0)
paste -s -d "${delim_euro}\\0" f3 > out || fail=1
printf "1${delim_euro}23\n" > exp || framework_failure_
compare exp out || fail=1

# Test invalid UTF-8 sequences are still passed through
delims_invalid=$(bad_unicode)
delim_invalid=$(env printf '%s' "$delims_invalid" | cut -b1)
paste -d "$delims_invalid" f1 f2 > out || fail=1
printf "1${delim_invalid}a\n2${delim_invalid}b\n" > exp || framework_failure_
compare exp out || fail=1

# Test that \<multi-byte char> is treated like <multi-byte char>
# (unknown escapes pass through the escaped character)
paste -d "\\${delim_euro}" f1 f2 > out || fail=1
paste -d "$delim_euro" f1 f2 > exp || fail=1
compare exp out || fail=1


# Test GB18030 encoding if available
export LC_ALL=zh_CN.gb18030

if test "$(locale charmap 2>/dev/null | sed 's/gb/GB/')" = GB18030; then
  # GB18030 2-byte character (e.g., 0xA2 0xE3 is a valid GB18030 char)
  delim_gb18030=$(env printf '\xa2\xe3')

  paste -d "$delim_gb18030" f1 f2 > out || fail=1
  printf "1${delim_gb18030}a\n2${delim_gb18030}b\n" > exp || framework_failure_
  compare exp out || fail=1

  paste -s -d "$delim_gb18030" f3 > out || fail=1
  printf "1${delim_gb18030}2${delim_gb18030}3\n" > exp || framework_failure_
  compare exp out || fail=1

  # Note 0xFF is invalid in GB18030, but we support all single byte delimiters
  delim_ff=$(env printf '\xff')
  paste -d "$delim_ff" f1 f2 > out || fail=1
  printf "1${delim_ff}a\n2${delim_ff}b\n" > exp || framework_failure_
  compare exp out || fail=1
fi

Exit $fail
