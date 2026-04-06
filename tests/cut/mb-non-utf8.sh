#!/bin/sh
# Test cut with non-UTF-8 multibyte locales.

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
print_ver_ cut printf

export LC_ALL=zh_CN.gb18030

test "$(locale charmap 2>/dev/null | sed 's/gb/GB/')" = GB18030 ||
  skip_ 'GB18030 charset support not detected'

delim_gb18030=$(env printf '\xa2\xe3')
delim_ff=$(env printf '\xff')

# Exercise ASCII, multibyte, and invalid single-byte delimiters.
# Note 0xFF is invalid in GB18030, but we support all single byte delimiters.
for delim in ',' ':' "$delim_gb18030" "$delim_ff"; do
  num_out=$(printf "1${delim}2${delim}3\n" \
            | cut -d "$delim" -f2,3 --output-delimiter=_)
  test "$num_out" = "2_3" || fail=1
done

# A valid 2-byte GB18030 character.
printf '%sx\n' "$delim_gb18030" | cut -c1 > out || fail=1
printf '%s\n' "$delim_gb18030" > exp || framework_failure_
compare exp out || fail=1

printf '%sx\n' "$delim_gb18030" | cut -c2 > out || fail=1
printf 'x\n' > exp || framework_failure_
compare exp out || fail=1

printf '%sx\n' "$delim_gb18030" | cut -b1 -n > out || fail=1
printf '\n' > exp || framework_failure_
compare exp out || fail=1

printf '%sx\n' "$delim_gb18030" | cut -b2 -n > out || fail=1
printf '%s\n' "$delim_gb18030" > exp || framework_failure_
compare exp out || fail=1

printf '1%s2%s3\n' "$delim_gb18030" "$delim_gb18030" \
  | cut -d "$delim_gb18030" -f1,3 > out || fail=1
printf '1%s3\n' "$delim_gb18030" > exp || framework_failure_
compare exp out || fail=1

printf '1%s2%s3\n' "$delim_gb18030" "$delim_gb18030" \
  | cut --complement -d "$delim_gb18030" -f1 > out || fail=1
printf '2%s3\n' "$delim_gb18030" > exp || framework_failure_
compare exp out || fail=1

printf '1%s2' "$delim_gb18030" \
  | cut -d "$delim_gb18030" -f2 > out || fail=1
printf '2\n' > exp || framework_failure_
compare exp out || fail=1

printf '1%s2\0' "$delim_gb18030" \
  | cut -z -d "$delim_gb18030" -f2 > out || fail=1
printf '2\0' > exp || framework_failure_
compare exp out || fail=1

printf '%s%s3\n' "$delim_gb18030" "$delim_gb18030" \
  | cut -d "$delim_gb18030" -f1-3 --output-delimiter=':' > out || fail=1
printf '::3\n' > exp || framework_failure_
compare exp out || fail=1

Exit $fail
