#!/bin/sh
# Test fold with zero width characters.

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
print_ver_ fold printf
getlimits_

# Make sure we do not overflow the buffer.
IO_BUFSIZE_TIMES2=$(($IO_BUFSIZE * 2))

# Fold counts by columns by default.
head -c $IO_BUFSIZE_TIMES2 /dev/zero | fold > out || fail=1
test $(cat out | wc -l) -eq 0 || fail=1

# Check that zero width characters are counted with --characters.
head -c $IO_BUFSIZE_TIMES2 /dev/zero | fold --characters > out || fail=1
test $(cat out | wc -l) -eq $(($IO_BUFSIZE_TIMES2 / 80)) || fail=1

test "$LOCALE_FR_UTF8" != none || skip_ "French UTF-8 locale not available"

LC_ALL=$LOCALE_FR_UTF8
export LC_ALL

# Same thing, but using U+200B ZERO WIDTH SPACE.
yes $(env printf '\u200B') | head -n $IO_BUFSIZE_TIMES2 | tr -d '\n' > inp

fold inp > out || fail=1
test $(cat out | wc -l) -eq 0 || fail=1

fold --characters inp > out || fail=1
test $(cat out | wc -l) -eq $(($IO_BUFSIZE_TIMES2 / 80)) || fail=1

# Ensure bounded memory operation.
vm=$(get_min_ulimit_v_ fold /dev/null) && {
  head -c $IO_BUFSIZE_TIMES2 /dev/zero | tr -d '\n' \
    | (ulimit -v $(($vm+8000)) && fold 2>err) | head || fail=1
  compare /dev/null err || fail=1
}

Exit $fail
