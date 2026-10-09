#!/bin/sh
# Test rm's behavior when the directory cannot be read.

# Copyright (C) 2016-2026 Free Software Foundation, Inc.

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
print_ver_ rm
getlimits_
uses_strace_

mkdir -p dir/notempty || framework_failure_

rm_getdents_fail() {
  strace -o strace.out \
    -e trace=getdents64 \
    -e fault=getdents64:error=EIO:when="$@" \
    rm -Rf dir
}

# failure to read any items from dir, then assume empty.
# Generally that will be diagnosed when rm tries to rmdir().
echo "rm: cannot remove 'dir': $EIO" > exp || framework_failure_
rm_getdents_fail 1 2>err
ret=$?
grep getdents64 strace.out || skip_ 'getdents64 is not intercepted'
compare exp err || fail=1
test "$ret" = 1 || fail=1

# Second case is more general error where we fail immediately
# (with ENOENT in this case but it could be anything).
# e.g. 1st getdents64 cache was exhaused and 2nd getdents64 failed in the one readdir call
echo "rm: traversal failed: dir: $EIO" > exp || framework_failure_
rm_getdents_fail 2 2>err
ret=$?
# already checked strace support
compare exp err || fail=1
test "$ret" = 1 || fail=1

Exit $fail
