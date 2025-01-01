#!/bin/sh
# If ls is asked to list a removed directory (e.g., the parent process's
# current working directory has been removed by another process), it
# should not emit an error message merely because the directory is removed.

# Copyright (C) 2020-2025 Free Software Foundation, Inc.

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
print_ver_ ls

cwd=$(pwd)
mkdir d || framework_failure_
cd d || framework_failure_
rmdir ../d || skip_ "can't remove working directory on this platform"

# On NFS, 'ls' would run into the error "Stale file handle".
test -d . || skip_ "can't examine removed working directory on this platform"

ls >"$cwd"/out 2>"$cwd"/err || fail=1
cd "$cwd" || framework_failure_

compare /dev/null out || fail=1
compare /dev/null err || fail=1

Exit $fail
