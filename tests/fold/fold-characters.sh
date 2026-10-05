#!/bin/sh
# Test fold --characters.

# Copyright (C) 2025-2026 Free Software Foundation, Inc.

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

{ test -z "$LOCALE_FR_UTF8" || test "$LOCALE_FR_UTF8" = none; } &&
  skip_ 'French UTF-8 locale not available'

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

bad_unicode_with_nul () { env printf '%s|\u0000\n' "$(bad_unicode)"; }
bad_unicode_with_nul > exp4 || framework_failure_
bad_unicode_with_nul | fold > out4 || fail=1
compare exp4 out4 || fail=1

# Check bad character at EOF
test $(env printf '\xC3' | fold | wc -c) = 1 || fail=1

# Ensure backspace clamps at position 0
# From v9.8 to v9.12 inclusive, an internal unsigned could wrap,
# causing premature line wrapping.
test $(env printf 'A\uB250\b\b\b\bB\n' | fold -w80 | wc -l) = 1 || fail=1

# A backspace after an initial tab must move back one column.
env printf '\t\bX\n' > exp5 || framework_failure_
fold -w8 exp5 > out5 || fail=1
compare exp5 out5 || fail=1
fold --characters -w8 exp5 > out5 || fail=1
compare exp5 out5 || fail=1

# A tab must reset the saved width after a wide character.
env printf '\uB250\t\bXX\n' > input6 || framework_failure_
env printf '\uB250\t\bX\nX\n' > exp6 || framework_failure_
fold -w8 input6 > out6 || fail=1
compare exp6 out6 || fail=1

# Likewise after a zero width combining character.
env printf 'a\u0301\t\bX\n' > exp7 || framework_failure_
fold -w8 exp7 > out7 || fail=1
compare exp7 out7 || fail=1

# A backspace moves one column, even after a two column character.
env printf '\uB250\bXX\n' > input8 || framework_failure_
env printf '\uB250\bX\nX\n' > exp8 || framework_failure_
fold -w2 input8 > out8 || fail=1
compare exp8 out8 || fail=1
# In character mode, the wide character and backspace cancel out.
fold --characters -w2 input8 > out8 || fail=1
compare input8 out8 || fail=1

# A backspace moves one column, even after a zero width character.
env printf 'a\u0301\bXX\n' > input9 || framework_failure_
fold -w2 input9 > out9 || fail=1
compare input9 out9 || fail=1
# In character mode, the combining character also counts as one.
env printf 'a\u0301\bX\nX\n' > exp9 || framework_failure_
fold --characters -w2 input9 > out9 || fail=1
compare exp9 out9 || fail=1

Exit $fail
