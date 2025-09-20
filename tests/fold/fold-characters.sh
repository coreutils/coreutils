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
getlimits_

test "$LOCALE_FR_UTF8" != none || skip_ "French UTF-8 locale not available"

LC_ALL=$LOCALE_FR_UTF8
export LC_ALL

test $(env printf '\uB250\uFF1A' | wc -L) -eq 4 ||
  skip_ "character width mismatch"

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

# Test a Unicode character on the edge of the input buffer.
# Keep in sync with IO_BUFSIZE - 1.
IO_BUFSIZE_MINUS_1=$(($IO_BUFSIZE - 1))
test $IO_BUFSIZE_MINUS_1 -gt 0 || framework_failure_
yes a | head -n $IO_BUFSIZE_MINUS_1 | tr -d '\n' > input3 || framework_failure_
env printf '\uB250' >> input3 || framework_failure_
yes a | head -n 100 | tr -d '\n' >> input3 || framework_failure_
env printf '\n' >> input3 || framework_failure_

yes a | head -n 80 | tr -d '\n' > exp3 || framework_failure_
env printf '\n' >> exp3 || framework_failure_
yes a | head -n 63 | tr -d '\n' >> exp3 || framework_failure_
env printf '\uB250' >> exp3 || framework_failure_
yes a | head -n 16 | tr -d '\n' >> exp3 || framework_failure_
env printf '\n' >> exp3 || framework_failure_
yes a | head -n 80 | tr -d '\n' >> exp3 || framework_failure_
env printf '\naaaa\n' >> exp3 || framework_failure_

fold --characters input3 | tail -n 4 > out3 || fail=1
compare exp3 out3 || fail=1

# Sequence derived from <https://datatracker.ietf.org/doc/rfc9839>.
bad_unicode ()
{
  # invalid UTF8|unpaired surrogate|NUL|C1 control|noncharacter
  env printf '\xC3|\xED\xBA\xAD|\u0000|\u0089|\xED\xA6\xBF\xED\xBF\xBF\n'
}
bad_unicode > /dev/null || framework_failure_
test $({ bad_unicode | fold; bad_unicode; } | uniq | wc -l) = 1 || fail=1
# Check bad character at EOF
test $(env printf '\xC3' | fold | wc -c) = 1 || fail=1

Exit $fail
