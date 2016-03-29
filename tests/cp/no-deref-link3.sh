#!/bin/sh
# cp from 3.16 fails this test

# Copyright (C) 1997-2016 Free Software Foundation, Inc.

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
print_ver_ cp

msg=bar
echo $msg > a
ln -s a b


# It should fail with a message something like this:
#   cp: 'a' and 'b' are the same file
cp -d a b 2>/dev/null

# Fail this test if the exit status is not 1
test $? = 1 || fail=1

test "$(cat a)" = $msg || fail=1

Exit $fail
