#!/bin/sh
# tests for printing multi-byte values of characters

# Copyright (C) 2022-2025 Free Software Foundation, Inc.

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
print_ver_ printf

prog='env printf'

unset LC_ALL
if test "$LOCALE_FR_UTF8" != "none"; then
  f=$LOCALE_FR_UTF8
  (
   #valid multi-byte
   LC_ALL=$f $prog '%04x\n' '"á' >>out 2>>err
   #invalid multi-byte
   LC_ALL=$f $prog '%04x\n' "'$($prog '\xe1')" >>out 2>>err
   #uni-byte
   LC_ALL=C $prog '%04x\n' "'$($prog '\xe1')" >>out 2>>err
   #valid multi-byte, with trailing
   LC_ALL=$f $prog '%04x\n' '"á=' >>out 2>>err
   #invalid multi-byte, with trailing
   LC_ALL=$f $prog '%04x\n' "'$($prog '\xe1')=" >>out 2>>err
  )
  cat <<\EOF > exp || framework_failure_
00e1
00e1
00e1
00e1
00e1
EOF
  compare exp out || fail=1

  # Disparate LC_CTYPE and LC_MESSAGES problematic on macos,
  # so just look for character in warning message,
  # and normalize to LC_MESSAGES=C
  C_WARNING='printf: '\
'warning: =: character(s) following character constant have been ignored'

  sed "s/printf:.*=.*/$C_WARNING/" < err > c_err || framework_failure_

  cat <<EOF > exp_err
$C_WARNING
$C_WARNING
EOF
  compare exp_err c_err || fail=1
fi

Exit $fail
