#!/bin/sh
# Test fold --spaces with various Unicode breaking space characters.

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
print_ver_ fold printf

test "$LOCALE_FR_UTF8" != none || skip_ "French UTF-8 locale not available"

LC_ALL=$LOCALE_FR_UTF8
export LC_ALL

# Check if a character is considered blank.  Some systems, such as FreeBSD,
# only consider ASCII spaces and tabs blank.
isblank ()
{
  test $(printf "a$1a\nb$1b\n" | uniq -f1 | wc -l) = 2;
}

if isblank '\u2002'; then
  # One U+2002 EN SPACE characters.
  env printf 'abcdefghijklmnop\u2002qrstuvwxyz\n' > input1 \
    || framework_failure_
  env printf 'abcdefghij\nklmnop\u2002\nqrstuvwxyz\n'> exp1 \
    || framework_failure_
  fold --spaces --width 10 input1 > out1 || fail=1
  compare exp1 out1 || fail=1
fi

if isblank '\u2003'; then
  # Two U+2003 EM SPACE characters.
  env printf 'abcdefghijklmnop\u2003\u2003qrstuvwxyz\n' > input2 \
    || framework_failure_
  env printf 'abcdefghij\nklmnop\u2003\u2003\nqrstuvwxyz\n'> exp2 \
    || framework_failure_
  fold --spaces --width 10 input2 > out2 || fail=1
  compare exp2 out2 || fail=1
fi

Exit $fail
