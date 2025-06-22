#!/bin/sh
# Ensure that stty diagnoses invalid inputs, rather than silently misbehaving.

# Copyright (C) 2007-2025 Free Software Foundation, Inc.

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
print_ver_ stty
require_controlling_input_terminal_
require_trap_signame_
getlimits_

trap '' TTOU # Ignore SIGTTOU


saved_state=$(stty -g) || fail=1
stty $saved_state || fail=1

# Before coreutils-6.9.90, if stty were given an argument with 35 colons
# separating 36 hexadecimal strings, stty would fail to diagnose as invalid
# any number that was out of range as long as sscanf happened to
# overflow/wrap it back into the range of the corresponding type (either
# tcflag_t or cc_t).

# For each of the following, with coreutils-6.9 and earlier,
# stty would fail to diagnose the error on at least Solaris 10.
hex_2_64=10000000000000000
returns_ 1 stty $(echo $saved_state |sed 's/^[^:]*:/'$hex_2_64:/) \
  2>/dev/null || fail=1
returns_ 1 stty $(echo $saved_state |sed 's/:[0-9a-f]*$/:'$hex_2_64/) \
  2>/dev/null || fail=1

# From coreutils 5.3.0 to 8.28, the following would crash
# due to incorrect argument handling.
if tty -s </dev/tty; then
  returns_ 1 stty eol -F /dev/tty || fail=1
  returns_ 1 stty -F /dev/tty eol || fail=1
  returns_ 1 stty -F/dev/tty eol || fail=1
  returns_ 1 stty eol -F/dev/tty eol || fail=1
fi

# coreutils >= 9.8 supports arbitrary speeds on some systems
# so restrict tests here to invalid numbers
# We simulate unsupported numbers in a separate "LD_PRELOAD" test.
WRAP_9600="$(expr $ULONG_OFLOW - 9600)"
for speed in 9599.. 9600.. 9600.5. 9600.50. 9600.0. ++9600 \
             -$WRAP_9600 --$WRAP_9600 0x2580 96E2 9600,0 '9600.0 '; do
  returns_ 1 stty ispeed "$speed" || fail=1
done

# Just in case either of the above mistakenly succeeds (and changes
# the state of our tty), try to restore the initial state.
stty $saved_state || fail=1

Exit $fail
