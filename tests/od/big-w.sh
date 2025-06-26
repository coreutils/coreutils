#!/bin/sh
# Check whether od -wN works with big N

# Copyright 2025 Free Software Foundation, Inc.

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
print_ver_ od
very_expensive_

export LC_ALL=C

cat >exp <<'EOF' || framework_failure_
0000000 x >x<
0000001
EOF

# Try values near sqrt(2**31) and sqrt(2**63).
for w in 46340 46341 3037000500 3037000501; do
  printf x | od -w$w -tcz 2>err | tr -s ' ' ' ' >out
  if test -s err; then
    test ! -s out || fail=1
  else
    compare exp out || fail=1
    outbytes=$(printf x | od -w$w -tcz | wc -c)
    expbytes=$((4*$w + 21))
    test $expbytes -eq $outbytes || fail=1
  fi
done

Exit $fail
