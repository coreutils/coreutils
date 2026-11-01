#!/bin/sh
# Basic tests for 'mkfifo'.

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
print_ver_ mkfifo
getlimits_

# Ensure fifos are supported.
mkfifo_or_skip_ fifo

for mode in '' 600; do
  mode_arg="--mode $mode"
  test -z "$mode" && mode_arg=

  mkdir -p d1 d2 || framework_failure_

  # Single non-existent operand.
  for name in d1/f1 d2/f1; do
    mkfifo $mode_arg $name >out 2>err || fail=1
    compare /dev/null out || fail=1
    compare /dev/null err || fail=1
    if test -n "$mode"; then
      test "$(stat --format=%a $name)" = $mode || fail=1
    fi
  done

  # Two non-existent operands.
  mkfifo $mode_arg d1/f2 d2/f2 >out 2>err || fail=1
  compare /dev/null out || fail=1
  compare /dev/null err || fail=1
  if test -n "$mode"; then
    test "$(stat --format=%a d1/f2)" = $mode || fail=1
    test "$(stat --format=%a d2/f2)" = $mode || fail=1
  fi

  # Mix of non-existent operands, existent-operands,
  # and one missing its parent.
  cat <<EOF >exp-err || framework_failure_
mkfifo: cannot create fifo 'd1/f1': $EEXIST
mkfifo: cannot create fifo 'd1/f2': $EEXIST
mkfifo: cannot create fifo 'd2/f1': $EEXIST
mkfifo: cannot create fifo 'd2/f2': $EEXIST
mkfifo: cannot create fifo 'missing/f1': $ENOENT
EOF
  returns_ 1 mkfifo $mode_arg d1/f1 d2/f3 d1/f2 d2/f1 d2/f2 missing/f1 d1/f3 \
    >out 2>err || fail=1
  compare /dev/null out || fail=1
  compare exp-err err || fail=1
  if test -n "$mode"; then
    test "$(stat --format=%a d1/f3)" = $mode || fail=1
    test "$(stat --format=%a d2/f3)" = $mode || fail=1
  fi

  rm -rf d1 d2 || framework_failure_
done

Exit $fail
