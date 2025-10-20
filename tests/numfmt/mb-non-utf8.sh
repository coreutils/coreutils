#!/bin/sh
# Test handling of non-utf8 locales

# Copyright (C) 2025 Free Software Foundation, Inc.

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
print_ver_ numfmt printf

export LC_ALL=zh_CN.gb18030

test "$(locale charmap 2>/dev/null | sed 's/gb/GB/')" = GB18030 ||
  skip_ 'GB18030 charset support not detected'

# Requires support for strchr(), mbschr(), and mbsstr() respectively.
# Note 0xFF is invalid in GB18030, but we support all single byte delimiters.
for delim in ',' ':' "$(env printf '\xa2\xe3')" "$(env printf '\xff')"; do
  num_out=$(numfmt --from=si --field=2 -d "$delim" "1${delim}2K")
  test "$num_out" = "1${delim}2000" || fail=1
done

Exit $fail
