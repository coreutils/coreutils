#!/bin/sh
# basic tests for echo

# Copyright (C) 2018 Free Software Foundation, Inc.

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

prog='env echo'

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ echo


# Verify the methods of specifying "Escape":
# Note 4 octal digits are allowed (unlike printf which uses up to 3)
printf '%s\n' . . . . . | tr . '\033' > exp
$prog -n -e '\x1b\n\e\n\33\n\033\n\0033\n' > out || fail=1
compare exp out || fail=1

# Incomplete hex escapes are output as is (unlike printf)
printf '\\x\n' > exp
$prog -n -e '\x\n' > out || fail=1
compare exp out || fail=1

# Always output -- (unlike printf)
$prog -- 'foo' > out || fail=1
$prog -n -e -- 'foo\n' >> out || fail=1
cat <<\EOF > exp
-- foo
-- foo
EOF
compare exp out || fail=1

# Ensure \c stops processing
$prog -e 'foo\n\cbar' > out || fail=1
printf 'foo\n' > exp
compare exp out || fail=1

# With POSIXLY_CORRECT:
#   only -n as the first (separate) option enables option processing
#   -E is ignored
#   escapes are processed by default
POSIXLY_CORRECT=1 $prog -n -E 'foo\n' > out || fail=1
POSIXLY_CORRECT=1 $prog -nE 'foo' >> out || fail=1
POSIXLY_CORRECT=1 $prog -E -n 'foo' >> out || fail=1
POSIXLY_CORRECT=1 $prog --version >> out || fail=1
cat <<\EOF > exp
foo
-nE foo
-E -n foo
--version
EOF
compare exp out || fail=1

Exit $fail
