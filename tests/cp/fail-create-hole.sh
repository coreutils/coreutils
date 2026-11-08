#!/bin/sh
# Test that 'cp' behaves correctly when failing to create a hole.

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
print_ver_ cp
require_sparse_support_
require_strace_ fallocate
getlimits_

fail_fallocate ()
{
  strace --quiet=all -o trace -e inject=fallocate:error=EIO "$@" >out 2>err
}

# Make sure we can inject an fallocate failure.
fail_fallocate true || skip_ 'strace does not support the required options'

# Create a file starting with "a", followed by zeros and then "b".
{ printf a && dd if=/dev/zero bs=256k count=1 status=none && printf b; } >in &&
echo "cp: error deallocating 'copy': $EIO" >exp-err || framework_failure_

fail_fallocate cp --reflink=never --sparse=always in copy
rc=$?

grep 'EIO .*(INJECTED)' trace || skip_ 'cannot intercept fallocate'

test $rc -eq 1 || fail=1
compare /dev/null out || fail=1
compare exp-err err || fail=1
test -f copy || fail=1

Exit $fail
