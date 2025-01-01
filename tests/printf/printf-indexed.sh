#!/bin/sh
# tests for printf %i$ indexed format processing

# Copyright (C) 2024-2025 Free Software Foundation, Inc.

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

getlimits_

prog='env printf'

printf_check() {
  cat <<EOF > exp || framework_failure_
$1
EOF

  shift

  $prog "$@" > out || fail=1
  compare exp out || fail=1
}

printf_check_err() {
  cat <<EOF > exp || framework_failure_
$1
EOF

  shift

  returns_ 1 $prog "$@" 2> out || fail=1
  compare exp out || fail=1
}

NL="
"

# Reordering
printf_check '21' '%2$s%1$s\n' 1 2

# Repetition
printf_check "11${NL}22" '%1$s%1$s\n' 1 2

# Multiple uses of format
printf_check "A C B${NL}D  " '%s %3$s %s\n' A B C D
printf_check "   4${NL}1" '%1$*d\n' 4 1

# Mixed indexed and sequential main arg
printf_check "A B A" '%s %s %1$s\n' A B
printf_check '   0 1  ' '%100$*d %s %s %s\n' 4 1

# indexed arg, width, and precision
printf_check ' 01' '%1$*2$.*3$d\n' 1 3 2
# indexed arg, sequential width, and precision
printf_check ' 01' '%3$*.*d\n' 3 2 1
# indexed arg, width, and sequential precision
printf_check ' 01' '%3$*2$.*d\n' 2 3 1
# indexed arg, precision, and sequential width
printf_check ' 01' '%3$*.*2$d\n' 3 2 1
# Indexed arg, width
printf_check '   1' '%2$*1$d\n' 4 1
# Indexed arg, and sequential width
printf_check '   1' '%2$*d\n' 4 1

# Flags come after $ (0 is not a flag here but allowed):
printf_check '   1' '%01$4d\n' 1
# Flags come after $ (0 is a flag here):
printf_check '0001' '%1$0*2$d\n' 1 4
# Flags come after $ (-2 not taken as a valid index here):
printf_check_err 'printf: %-2$: invalid conversion specification' \
                 '%-2$s %1$s\n' A B
# Flags come after $ (' ' is not allowed as part of number here)
printf_check_err 'printf: % 2$: invalid conversion specification' \
                 '% 2$s %1$s\n' A B

# Ensure only base 10 numbers are accepted
printf_check_err "printf: 'A': expected a numeric value" \
                 '%0x2$s %2$s\n' A B
# Ensure empty numbers are rejected
printf_check_err 'printf: %$: invalid conversion specification' \
                 '%$d\n' 1
# Verify int limits are clamped appropriately (to INT_MAX)
# (Large indexes are useful to ensure a single pass with the format arg)
# Note you can't have more than INT_MAX - 2 args anyway as argc is an int,
# and that also includes the command name and format at least.
for i in 999 $INT_MAX $INT_OFLOW $INTMAX_MAX $INTMAX_OFLOW; do
  printf_check 'empty' "empty%$i\$s\n" 'foo'
done

Exit $fail
