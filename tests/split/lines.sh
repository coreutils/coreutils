#!/bin/sh
# show that 'split --lines=2' works.

# Copyright (C) 2002-2026 Free Software Foundation, Inc.

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
print_ver_ split

printf '1\n2\n3\n4\n5\n' > in || framework_failure_

split --lines=2 in > out || fail=1
cat <<\EOF > exp-1
1
2
EOF
cat <<\EOF > exp-2
3
4
EOF
cat <<\EOF > exp-3
5
EOF

compare exp-1 xaa || fail=1
compare exp-2 xab || fail=1
compare exp-3 xac || fail=1
test -f xad && fail=1

# The line count is a plain decimal number: a leading zero is not octal,
# and neither the obsolete -N form nor -l accepts size suffixes or hex.
seq 10 > in10 || framework_failure_
for opt in '-l 010' '-010' '-l +10' "-l ' 10'"; do
  rm -f x?? || framework_failure_
  eval split $opt in10 || fail=1
  compare in10 xaa || fail=1
  test -f xab && fail=1
done

for n in 1k 1K 0x10 1e2 5b; do
  returns_ 1 split -l $n in10 2>err || fail=1
  echo "split: invalid number of lines: '$n'" > exp-err || framework_failure_
  compare exp-err err || fail=1
done

Exit $fail
