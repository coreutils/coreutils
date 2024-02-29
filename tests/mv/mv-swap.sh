#!/bin/sh
# Test whether mv -x,--swap swaps targets

# Copyright (C) 2024 Free Software Foundation, Inc.

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
print_ver_ mv


# test swapping files
touch a || framework_failure_
mkdir b || framework_failure_
if ! mv -x a b 2>swap_err; then
  grep 'not supported' swap_err || { cat swap_err; fail=1; }
else
  test -d a || fail=1
  test -f b || fail=1
fi

# test wrong number of arguments
touch c || framework_failure_
returns_ 1 mv --swap a 2>/dev/null || fail=1
returns_ 1 mv --swap a b c 2>/dev/null || fail=1

# both files must exist
returns_ 1 mv --swap a d 2>/dev/null || fail=1

# swapping can't be used with -t or -u
mkdir d
returns_ 1 mv --swap -t d a b 2>/dev/null || fail=1
returns_ 1 mv --swap -t d a 2>/dev/null || fail=1
returns_ 1 mv --swap -u a b 2>/dev/null || fail=1

Exit $fail
