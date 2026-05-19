#!/bin/sh
# Test 'tee' when a write fails with errno set to EAGAIN.

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
require_strace_ write

# In coreutils-9.11 the following test would infinite loop.
echo a >exp || framework_failure_
timeout 10 strace -qqq -o /dev/null -e trace-fds=3 \
  -e inject=write:error=EAGAIN:when=1 tee file1 <exp >out 2>err || fail=1
compare exp file1 || fail=1
compare exp out || fail=1
compare /dev/null err || fail=1

Exit $fail
