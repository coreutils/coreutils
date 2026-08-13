#!/bin/sh
# Check that 'dd' does not continue copying if ftruncate and fstat fail.

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
getlimits_

# Setup the file "out" and preserve it's original contents in "exp-out".
yes | head -n 2048 | tr -d '\n' > out || framework_failure_
cp out exp-out || framework_failure_

strace -o strace.out \
  -e fault=ftruncate:error=EPERM \
  -e fault=fstat:error=EPERM \
  -P out \
  dd if=/dev/zero of=out count=1 seek=1 status=none 2>err

ret=$?

grep -oE '^(fstat|ftruncate)' strace.out || skip_ "ftruncate or fstat are not used"

# After ftruncate fails, we use fstat to get the file type.
echo "dd: cannot fstat 'out': $EPERM" > exp
compare exp err || fail=1

# coreutils 9.1 to 9.9 would mistakenly continue copying after ftruncate
# failed and exit successfully.
test "$ret" = 1 || fail=1
compare exp-out out || fail=1

Exit $fail
