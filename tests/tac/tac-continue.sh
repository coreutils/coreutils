#!/bin/sh
# Ensure that tac processes all command line arguments, even
# when it encounters an error with say the first one.
# With coreutils-5.2.1 and earlier, this test would fail.

# Copyright (C) 2004-2026 Free Software Foundation, Inc.

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
require_root_

cwd=$(pwd)
cleanup_() { cd /; umount "$cwd/full_tmp"; }

mkdir full_tmp || framework_failure_

mount -t tmpfs --options size=1M tmpfs $cwd/full_tmp ||
 skip_ 'Unable to mount small tmpfs'

# Make sure we can create an empty file there (i.e., no shortage of inodes).
touch "$cwd/full_tmp/tac-empty" || framework_failure_

seq 5 > five && seq 5 -1 1 > exp || framework_failure_

# Make sure we diagnose the failure but continue to subsequent files
yes | TMPDIR=$cwd/full_tmp timeout 10 tac - five >out 2>err && fail=1
{ test $? = 124 || ! grep 'space' err >/dev/null; } && fail=1
compare exp out || fail=1

Exit $fail
