#!/bin/sh
# Ensure that rm does not skip extra files after hitting an empty immutable dir.
# Requires root access to do chattr +i, as well as an ext[23] or xfs file system

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
print_ver_ rm
require_root_

# These simple one-file operations are expected to work even in the
# presence of this bug, and we need them to set up the rest of the test.
chattr_i_works=1
touch f
chattr +i f 2>/dev/null || chattr_i_works=0
rm f 2>/dev/null
test -f f || chattr_i_works=0
chattr -i f 2>/dev/null || chattr_i_works=0
rm f 2>/dev/null || chattr_i_works=0
test -f f && chattr_i_works=0

if test $chattr_i_works = 0; then
  skip_ "chattr +i doesn't work on this file system"
fi

mkdir empty || framework_failure_
touch x y || framework_failure_
chattr +i empty || framework_failure_
rm -rf empty x y
{ test -f x || test -f y; } && fail=1
chattr -i empty

Exit $fail
