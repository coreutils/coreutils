#!/bin/sh
# Test 'tty'.

# Copyright 2017-2025 Free Software Foundation, Inc.

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

# Make sure there's a tty on stdin.
. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ tty

tty_works_on_stdin=false

if test -t 0; then
  if tty; then
    tty_works_on_stdin=true
  else
    test $? -eq 4 || fail=1
  fi
  tty -s || fail=1
fi

returns_ 1 tty </dev/null || fail=1
returns_ 1 tty -s </dev/null || fail=1
returns_ 1 tty <&- 2>/dev/null || fail=1
returns_ 1 tty -s <&- || fail=1

returns_ 2 tty a || fail=1
returns_ 2 tty -s a || fail=1

if test -w /dev/full && test -c /dev/full; then
  if $tty_works_on_stdin; then
    returns_ 3 tty >/dev/full || fail=1
  fi
  returns_ 3 tty </dev/null >/dev/full || fail=1
fi

Exit $fail
