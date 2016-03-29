#!/bin/sh
# exercise nl functionality

# Copyright (C) 2002-2016 Free Software Foundation, Inc.

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
print_ver_ nl


echo a | nl > out || fail=1
echo b | nl -s%n >> out || fail=1
echo c | nl -n ln >> out || fail=1
echo d | nl -n rn >> out || fail=1
echo e | nl -n rz >> out || fail=1
echo === >> out
printf 'a\n\n' | nl > t || fail=1; cat -A t >> out
cat <<\EOF > exp
     1	a
     1%nb
1     	c
     1	d
000001	e
===
     1^Ia$
       $
EOF

compare exp out || fail=1

Exit $fail
