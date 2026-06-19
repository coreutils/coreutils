#!/bin/sh
# A single input byte that decodes to a wide character must not overrun
# wc's byte-indexed isprint/isspace tables.

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
print_ver_ wc printf

# In SHIFT-JIS the bytes 0xA1..0xDF are half-width katakana: each is a
# single input byte, yet it decodes to U+FF61..U+FF9F, i.e. > UCHAR_MAX.
# wc indexes its 256-entry wc_isprint[]/wc_isspace[] tables with the
# decoded value, so before this fix it read past their end and crashed.
export LC_ALL=ja_JP.SHIFT_JIS

# No distribution ships this locale, so generate it locally if needed.
if test "$(locale charmap 2>/dev/null)" != SHIFT_JIS; then
  mkdir locale || framework_failure_
  localedef --no-archive --no-warnings=ascii \
    -f SHIFT_JIS -i ja_JP "$PWD/locale/ja_JP.SHIFT_JIS" \
    >/dev/null 2>&1 || skip_ "SHIFT-JIS locale not available"
  export LOCPATH="$PWD/locale"
fi

env printf 'a\xa1b\n' > in || framework_failure_

# 0xA1 is one byte and one non-space character: 1 line, 1 word, 4 bytes.
wc in > out || fail=1
printf '1 1 4 in\n' > exp || framework_failure_
compare exp out || fail=1

# -L reaches the same table through wc_isprint; ensure it too completes.
wc -L in > /dev/null || fail=1

Exit $fail
