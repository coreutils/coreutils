#!/bin/sh
# ensure that tac works with non-seekable or quasi-seekable inputs

# Copyright (C) 2011-2026 Free Software Foundation, Inc.

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
print_ver_ tac

echo x | tac - - > out 2> err || fail=1
echo x > exp || framework_failure_
compare exp out || fail=1
compare /dev/null err || fail=1

# Make sure it works on funny files in /proc and /sys.
# The BTF file can return a short read before EOF.

for file in /proc/version /sys/kernel/profiling /sys/kernel/btf/vmlinux; do
  if test -r $file; then
    cp -f $file copy &&
    tac copy > exp1 || framework_failure_

    tac $file > out1 || fail=1
    compare exp1 out1 || fail=1
  fi
done

# On Linux, SEEK_END on /proc/self/environ returns 0.
# Give tac an environment entry likely to span a full read buffer.
if test -r /proc/self/environ; then
  tac_exe=$(cmd_exe tac) &&
  entry=$(printf 'PAD=%09000d' 0) || framework_failure_

  env -i "$entry" "$tac_exe" /proc/self/environ > out || fail=1
  # Count the entry and its terminating NUL, allowing extras added on exec.
  test "$(wc -c < out)" -ge "$(printf '%s\0' "$entry" | wc -c)" || fail=1
fi

# Assume timeout is due to failure to close stdin with <&-
# which was seen on NetBSD 7.1 / x86_64
returns_ 124 timeout 10 tac - - <&- 2>err && skip_ 'error closing stdin'
# This failed due to heap corruption from v8.15-v8.25 inclusive.
returns_ 1 timeout 10 tac - - <&- 2>err || fail=1

Exit $fail
