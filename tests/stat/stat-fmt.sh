#!/bin/sh
# stat --format tests

# Copyright (C) 2003-2026 Free Software Foundation, Inc.

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
print_ver_ stat


# ensure that stat properly handles a format string ending with %
for i in $(seq 50); do
  fmt=$(printf "%${i}s" %)
  out=$(stat --form="$fmt" .)
  test "$out" = "$fmt" || fail=1
done


# ensure QUOTING_STYLE is honored by %N
touch "'" || framework_failure_
# Default since v8.25
stat -c%N \' >> out || fail=1
# Default before v8.25
QUOTING_STYLE=locale stat -c%N \' >> out || fail=1
cat <<\EOF >exp
"'"
'\''
EOF
compare exp out || fail=1

# ensure control characters in file names are escaped by %N
# using the default shell-escape quoting style.
nl='
'
fname="a${nl}${nl}b${nl}c"
touch "$fname" || framework_failure_
stat -c%N "$fname" > out || fail=1
# contiguous control characters are clumped into one $'...' escape.
cat <<\EOF >exp
'a'$'\n\n''b'$'\n''c'
EOF
compare exp out || fail=1

# Check the behavior with invalid values of QUOTING_STYLE.
for style in '' 'abcdef'; do
  QUOTING_STYLE="$style" stat -c%%%N \' > out 2> err || fail=1
  cat <<\EOF > exp-out || framework_failure_
%"'"
EOF
  cat <<EOF > exp-err || framework_failure_
stat: ignoring invalid value of environment variable QUOTING_STYLE: '$style'
EOF
  compare exp-out out || fail=1
  compare exp-err err || fail=1
  # coreutils-9.10 and earlier would unnecessarily check QUOTING_STYLE in
  # these cases.
  for format in '%%N' 'abc%%Ndef' 'abc%%Ndef%%N' '%%%%N'; do
    QUOTING_STYLE="$style" stat -c"$format" \' > out 2> err || fail=1
    printf "$format\n" > exp-out || framework_failure_
    compare exp-out out || fail=1
    compare /dev/null err || fail=1
  done
done

# Check the behavior with %N following %%N.
stat -cabc%%Ndef%N \' > out || fail=1
QUOTING_STYLE=locale stat -cabc%%Ndef%N \' >> out || fail=1
cat <<\EOF >exp
abc%Ndef"'"
abc%Ndef'\''
EOF
compare exp out || fail=1

# ensure %H and %L modifiers are handled
stat -c '%r %R %Hd,%Ld %Hr,%Lr' . > out || fail=1
grep -F '?' out && fail=1

Exit $fail
