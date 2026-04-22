#!/bin/sh
# Test that 'comm - -' works.

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
print_ver_ comm

printf '1\n\t2\n\t\t3\n4\n\t5\n\t\t6\n7\n\t8\n\t\t9\n' >exp \
  || framework_failure_

# In coreutils 9.11 and earlier, running 'comm - -' would close
# standard input twice, leading to an incorrect exit status.
seq 9 | sed 'n;n;p' | timeout 10 comm - - >out 2>err || fail=1
compare exp out || fail=1
compare /dev/null err || fail=1

Exit $fail
