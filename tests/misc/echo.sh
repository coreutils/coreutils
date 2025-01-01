#!/bin/sh
# basic tests for echo

# Copyright (C) 2018-2025 Free Software Foundation, Inc.

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

# Further test coverage.
# Output a literal '-' (on a line itself).
$prog - > out || fail=1
# Output a literal backslash '\', no newline.
$prog -n -e '\\' >> out || fail=1
# Output an empty line (merely to have a newline after the previous test).
$prog >> out || fail=1
# Test other characters escaped by a backslash:
# \a  hex 07  alert, bell
# \b  hex 08  backspace
# \e  hex 1b  escape
# \f  hex 0c  form feed
# \n  hex 0a  new line
# \r  hex 0d  carriage return
# \t  hex 09  horizontal tab
# \v  hex 0b  vertical tab
# Convert output, yet checking the exit status of $prog.
{ $prog -n -e '\a\b\e\f\n\r\t\v' || touch fail; } | od -tx1 >> out || fail=1
test '!' -f fail || fail=1
# Output hex values which contain hexadecimal characters to test hextobin().
# Hex values 4a through 4f are ASCII "JKLMNO".
$prog -n -e '\x4a\x4b\x4c\x4d\x4e\x4f\x4A\x4B\x4C\x4D\x4E\x4F' >> out || fail=1
# Output another newline.
$prog >> out || fail=1
cat <<\EOF > exp
-
\
0000000 07 08 1b 0c 0a 0d 09 0b
0000010
JKLMNOJKLMNO
EOF
compare exp out || fail=1

Exit $fail
