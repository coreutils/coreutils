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

for delim in ',' ':' "$(env printf '\xa2\xe3')" "$(env printf '\xff')"; do
  num_out=$(printf "1${delim}2${delim}3\n" \
            | cut -d "$delim" -f2,3 --output-delimiter=_)
  test "$num_out" = "2_3" || fail=1
done

Exit $fail
