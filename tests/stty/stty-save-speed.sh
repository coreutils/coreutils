#!/bin/sh
# Ensure saved terminal settings restore input and output speeds.

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
print_ver_ stty
getlimits_
require_controlling_input_terminal_
require_trap_signame_

# Linux encodes standard rates in c_cflag; BSD systems store all speeds
# separately.  Limit the format checks to these known layouts.
case $(uname -s) in
  Linux) separate_speeds=no;;
  FreeBSD | NetBSD | OpenBSD | DragonFly | Darwin) separate_speeds=yes;;
  *) skip_ 'requires Linux or BSD terminal speed layout';;
esac

trap '' TTOU # Ignore SIGTTOU

saved_state=$(stty -g) || framework_failure_
cleanup_ () { stty "$saved_state"; }

# Standard rates use the same number of fields regardless of their value.
stty 9600 || framework_failure_
state9600=$(stty -g) || framework_failure_
fields=$(echo "$state9600" | tr -cd : | wc -c) || framework_failure_
stty 19200 || fail=1
state=$(stty -g) || framework_failure_
test "$(echo "$state" | tr -cd : | wc -c)" = "$fields" || fail=1
stty "$state" || fail=1

# Restore standard speeds after changing them, including in one invocation.
for prefix in '' 19200; do
  stty 19200 || fail=1
  stty $prefix "$state9600" || fail=1
  stty speed > out || fail=1
  echo 9600 > exp || framework_failure_
  compare exp out || fail=1
done

# Restore distinct standard input and output speeds where supported.
if stty ospeed 9600 ispeed 19200 2>/dev/null; then
  state=$(stty -g) || framework_failure_
  stty speed > exp || fail=1
  stty 9600 || fail=1
  stty "$state" || fail=1
  stty speed > out || fail=1
  compare exp out || fail=1
fi
stty 19200 || fail=1

# On BSD, verify standard rates are restored after a speed change too.
# Retain an old-format string to test backward compatibility below.
legacy=$state9600
if test "$separate_speeds" = yes; then
  stty "$state9600" || fail=1
  stty speed > out || fail=1
  echo 9600 > exp || framework_failure_
  compare exp out || fail=1

  # Strip the last 2 fields
  fields=$((fields - 2))
  legacy=$(echo "$legacy" | cut -d: -f1-$(($fields + 1)))
fi

# Arbitrary rates require two additional fields.
for speeds in '51 51' '51 9600' '9600 51'; do
  set -- $speeds

  stty ospeed "$2" ispeed "$1" 2>/dev/null || continue
  stty speed > exp || fail=1
  state=$(stty -g) || framework_failure_

  # Ensure additional fields
  test "$(echo "$state" | tr -cd : | wc -c)" = $(($fields + 2)) || fail=1

  # Ensure speed restored from state
  stty 9600 || fail=1
  stty "$state" || fail=1
  stty speed > out || fail=1
  compare exp out || fail=1

  # Restore a standard-speed state directly after an arbitrary speed.
  stty "$state9600" || fail=1
  stty speed > out || fail=1
  echo 9600 > exp-standard || framework_failure_
  compare exp-standard out || fail=1
  stty "$state" "$state9600" || fail=1
  stty speed > out || fail=1
  compare exp-standard out || fail=1

  # The last specified speed takes precedence
  stty 19200 "$state" || fail=1
  stty speed > out || fail=1
  compare exp out || fail=1
  stty "$state" 9600 || fail=1
  stty speed > out || fail=1
  echo 9600 > exp || framework_failure_
  compare exp out || fail=1
done

# Reset to basic
stty 9600 || fail=1
stty "$legacy" || fail=1

# Test various invalid speeds.
# Both speed fields must be present, valid, and in range.
# Append a 0 (multiply by 16) to ULONG_MAX to ensure overflow.
overflow=$(printf '%x0' "$ULONG_MAX") || framework_failure_
for suffix in '' : 0 0: :0 0:0:0 "$overflow:0" \
              "0:$overflow" xyz:0 0:xyz; do
  returns_ 1 stty "$legacy:$suffix" 2>/dev/null || fail=1
done

Exit $fail
