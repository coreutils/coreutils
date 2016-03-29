#!/bin/sh
# Check that du can handle sub-bind-mounts cycles as well.

# Copyright (C) 2014-2016 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ du
require_root_

cleanup_() { umount a/b/c; }

mkdir -p a/b/c || framework_failure_
mount --bind a a/b/c \
  || skip_ 'This test requires mount with a working --bind option.'

echo a/b/c > exp || framework_failure_
echo a/b >> exp || framework_failure_

du a/b > out 2> err || fail=1
sed 's/^[0-9][0-9]*	//' out > k && mv k out

compare /dev/null err || fail=1
compare exp out || fail=1

Exit $fail
