#!/bin/sh
# Test 'cat' when a write is short.

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
print_ver_ cat
uses_strace_

# In tee of coreutils-9.11, a short write would be treated as an error.
# Verify same things for cat

if strace -f -o /dev/null -e inject=${syscalls}:error=ENOSYS true; then
  # splice:retval=1 does not consume content of pipe buffer.
  # len passed to splice(2) is depending on implementation, (large value or len of content of pipe buffer). So chech only 1st 2 bytes.
  printf ab > exp || framework_failure_
  echo abcdefg | strace -o /dev/null -qqq --trace-fds=1 -e trace=splice -e inject=splice:retval=1:when=1..5 cat | head -c 2 > out || fail=1
  compare exp out || fail=1
  # read consumes byte and write:retval=1 prevents writing it to stdout
  echo fg > exp || framework_failure_
  echo abcdefg | strace -o /dev/null -qqq --trace-fds=1 -e trace=splice,write -e fault=splice -e inject=write:retval=1:when=1..5 cat > out || fail=1
else
  echo fg > exp || framework_failure_
  echo abcdefg | strace -o /dev/null -qqq --trace-fds=1 -e trace=write -e inject=write:retval=1:when=1..5 cat > out || fail=1
fi

compare exp out || fail=1

Exit $fail
