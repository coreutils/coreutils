#!/bin/sh
# Test fold --characters.

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

# The string "뉐뉐뉐" is 3 characters, but occupies 6 columns.
env printf '\uB250\uB250\uB250\n' > input1 || framework_failure_
env printf '\uB250\uB250\n\uB250\n' > column-exp1 || framework_failure_

fold -w 5 input1 > column-out1 || fail=1
compare column-exp1 column-out1 || fail=1

# Should be the same as the input.
fold --characters -w 5 input1 > characters-out1 || fail=1
compare input1 characters-out1 || fail=1

# Test with 50 2 column wide characters.
for i in $(seq 50); do
  env printf '\uFF1A' >> input2 || framework_failure_
  env printf '\uFF1A' >> column-exp2 || framework_failure_
  env printf '\uFF1A' >> character-exp2 || framework_failure_
  if test $(($i % 5)) -eq 0; then
    env printf '\n' >> column-exp2 || framework_failure_
  fi
  if test $(($i % 10)) -eq 0; then
    env printf '\n' >> character-exp2 || framework_failure_
  fi
done

env printf '\n' >> input2 || framework_failure_

# 5 characters per line.
fold -w 10 input2 > column-out2 || fail=1
compare column-exp2 column-out2 || fail=1

# 10 characters per line.
fold --characters -w 10 input2 > character-out2 || fail=1
compare character-exp2 character-out2 || fail=1

Exit $fail
