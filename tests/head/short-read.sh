#!/bin/sh
# Exercise short reads when scanning backward on a seekable file.

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
print_ver_ head
getlimits_

file=/sys/kernel/btf/vmlinux  # Available in Linux >= 5.5
test -r "$file" || skip_ "cannot read $file"
size=$(stat -c %s "$file") || framework_failure_

# Limit the test to the last IO_BUFSIZE bytes of the file.
# Note head(1) currently uses $BUFSIZ buffers, but we use
# this larger size to decouple the test from the implementation.
window=$IO_BUFSIZE
test "$size" -ge "$window" || window=$size

from_suffix()
{
  (dd bs=1 iseek=$(($size - $window)) count=0 status=none && "$@") < "$file"
}

from_suffix dd bs="$IO_BUFSIZE" count=1 status=none > probe ||
  framework_failure_
n=$(wc -c < probe) || framework_failure_
test "$n" -gt 0 && test "$n" -lt "$window" ||
  skip_ "$file did not give a short read"

from_suffix cat > exp || framework_failure_
test "$(wc -c < exp)" -eq "$window" || framework_failure_
from_suffix head -n -0 > out || fail=1
compare exp out || fail=1

Exit $fail
