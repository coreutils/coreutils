#!/bin/sh
# Test that 'shred' behaves properly when failing to close the
# file descriptor used to write.

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
print_ver_ shred
require_strace_ close
getlimits_

echo a >a || framework_failure_

block_close ()
{
  strace --quiet=all -o /dev/null -P a -e inject=close:error=EIO "$@"
}

# Make sure we could inject a failed close on the selected path.
block_close true ||
  skip_ 'strace does not support the required options'

# Inject a failed close on the file we are shredding.
returns_ 1 block_close shred -n 1 -u a >out 2>err || fail=1

echo "shred: a: failed to close: $EIO" >exp-err || framework_failure_
compare /dev/null out || fail=1
compare exp-err err || fail=1

Exit $fail
