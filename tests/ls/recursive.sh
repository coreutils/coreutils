#!/bin/sh
# 4.1.1 and 4.1.2 had a bug whereby some recursive listings
# didn't include a blank line between per-directory groups of files.

# Copyright (C) 2001-2026 Free Software Foundation, Inc.

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

mkdir x y a b c a/1 a/2 a/3 || framework_failure_
touch f a/1/I a/1/II || framework_failure_


# This first example is from Andreas Schwab's bug report.
ls -R1 a b c > out || fail=1
cat <<EOF > exp
a:
1
2
3

a/1:
I
II

a/2:

a/3:

b:

c:
EOF

compare exp out || fail=1

rm -rf out exp
ls -R1 x y f > out || fail=1
cat <<EOF > exp
f

x:

y:
EOF

compare exp out || fail=1

# Check that we don't run out of file descriptors when visiting
# directories recursively.
mkdir -p $(seq 30 | tr '\n' '/') || framework_failure_
(ulimit -n 20 && ls -R 1 > out 2> err) || fail=1
test $(wc -l < out) = 88 || fail=1
test $(wc -l < err) = 0 || fail=1

Exit $fail
