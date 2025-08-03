#!/bin/sh
# Test readlink with POSIXLY_CORRECT defined.

# Copyright (C) 2025 Free Software Foundation, Inc.

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
print_ver_ readlink

touch file || framework_failure_
ln -s file link1 || framework_failure_

# POSIX requires a diagnostic error and non-zero exit status if the file is not
# a symbolic link.
cat <<\EOF > exp || framework_failure_
readlink: file: Invalid argument
EOF
returns_ 1 env POSIXLY_CORRECT=1 readlink file 2>err || fail=1
compare exp err || fail=1

# Does not occur for non-POSIX options.
env POSIXLY_CORRECT=1 readlink -f file || fail=1
env POSIXLY_CORRECT=1 readlink -e file || fail=1
env POSIXLY_CORRECT=1 readlink -m file || fail=1

# Check on a symbolic link.
cat <<\EOF > exp || framework_failure_
file
EOF
POSIXLY_CORRECT=1 readlink link1 >out || fail=1
compare exp out || fail=1
POSIXLY_CORRECT=1 readlink -f link1 || fail=1
POSIXLY_CORRECT=1 readlink -e link1 || fail=1
POSIXLY_CORRECT=1 readlink -m link1 || fail=1

Exit $fail
