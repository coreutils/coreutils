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
test $(wc -l < out) -eq 0 || fail=1

# Check that zero width characters are counted with --characters.
head -c $IO_BUFSIZE_TIMES2 /dev/zero | fold --characters > out || fail=1
test $(wc -l < out) -eq $(($IO_BUFSIZE_TIMES2 / 80)) || fail=1

test "$LOCALE_FR_UTF8" != none || skip_ "French UTF-8 locale not available"

LC_ALL=$LOCALE_FR_UTF8
export LC_ALL

test $(env printf '\u200B' | wc -L) -eq 0 ||
  skip_ "character width mismatch"

# Same thing, but using U+200B ZERO WIDTH SPACE.
yes $(env printf '\u200B') |
  head -n $IO_BUFSIZE_TIMES2 | tr -d '\n' > inp || framework_failure_

fold inp > out || fail=1
test $(wc -l < out) -eq 0 || fail=1

fold --characters inp > out || fail=1
test $(wc -l < out) -eq $(($IO_BUFSIZE_TIMES2 / 80)) || fail=1

# Ensure bounded memory operation.
test -w /dev/full && test -c /dev/full &&
vm=$(get_min_ulimit_v_ fold /dev/null) && {
  # \303 results in EILSEQ on input
  for c in '\n' '\0' '\303'; do
    tr '\0' "$c" < /dev/zero | timeout 10 $SHELL -c \
     "(ulimit -v $(($vm+12000)) && fold 2>err >/dev/full)"
    ret=$?
    test -f err || skip_ 'shell ulimit failure'
    { test $ret = 124 || ! grep 'space' err >/dev/null; } &&
     { fail=1; cat err; echo "fold didn't diagnose ENOSPC" >&2; }
  done
}

Exit $fail
