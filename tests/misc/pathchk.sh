#!/bin/sh
# pathchk tests

# Copyright (C) 2002-2026 Free Software Foundation, Inc.

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
print_ver_ pathchk
skip_if_root_

touch file || framework_failure_


# This should exit nonzero.  Before 2.0.13, it gave a diagnostic,
# but exited successfully.
returns_ 1 pathchk file/x > /dev/null 2>&1 || fail=1

# This tests the new -P option.
returns_ 1 pathchk -P -- - > /dev/null 2>&1 || fail=1
returns_ 1 pathchk -p -P x/- > /dev/null 2>&1 || fail=1

# Test empty file names with various options.  Through 5.3.0,
# 'pathchk -p' exited with status zero.
cat <<\EOF >exp || framework_failure_
pathchk: empty file name
EOF
for opt in -p -P '-p -P' --portability; do
  returns_ 1 pathchk $opt '' >out 2>err || fail=1
  compare /dev/null out || fail=1
  compare exp err || fail=1
done

Exit $fail
