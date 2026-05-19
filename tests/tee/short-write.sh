#!/bin/sh
# Test 'tee' when a write is short.

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
print_ver_ tee
uses_strace_

printf 'abcdef' >file1-exp || framework_failure_
printf 'f' >out-exp || framework_failure_

short_write() {
  strace -qqq -o /dev/null --trace-fds=1 -e trace=write \
    -e inject=write:retval=1:when=1..5 "$@"
}

short_write true || skip_ No functional strace found

# In coreutils-9.11, a short write would be treated as an error.
# Using 'returns_ 0' to avoid tracing impinging on stderr.
returns_ 0 short_write tee file1 >out 2>err <file1-exp || fail=1
compare file1-exp file1 || fail=1
compare out-exp out || fail=1
compare /dev/null err || fail=1

Exit $fail
