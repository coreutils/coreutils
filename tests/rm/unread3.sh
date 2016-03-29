#!/bin/sh
# Ensure that rm works even from an unreadable working directory.

# Copyright (C) 2004-2016 Free Software Foundation, Inc.

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
print_ver_ rm
skip_if_root_

mkdir -p a/1 b c d/2 e/3 || framework_failure_


t=$(pwd)
cd c
chmod u=x,go= .

# With coreutils-5.2.1, this would get a failed assertion.
rm -r "$t/a" "$t/b" || fail=1

# With coreutils-5.2.1, this would get the following:
#   rm: cannot get current directory: Permission denied
#   rm: failed to return to initial working directory: Bad file descriptor
rm -r "$t/d" "$t/e" || fail=1

test -d "$t/a" && fail=1
test -d "$t/b" && fail=1
test -d "$t/d" && fail=1
test -d "$t/e" && fail=1

Exit $fail
