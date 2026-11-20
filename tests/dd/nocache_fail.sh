#!/bin/sh
# Ensure we diagnose failure to drop caches
# We didn't check the return from posix_fadvise() correctly before v9.7

# Copyright (C) 2025-2026 Free Software Foundation, Inc.

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
print_ver_ dd
uses_strace_

touch ifile || framework_failure_

strace -o strace.out \
  -e trace=fadvise64 \
  -e fault=fadvise64:error=EOPNOTSUPP \
  dd if=ifile iflag=nocache count=0 2>err

ret=$?
grep fadvise64 strace.out || skip_ 'fadvise64 is not intercepted'
grep 'dd: failed to discard cache for: ifile' err || fail=1
test "$ret" = 1 || fail=1

Exit $fail
