#!/bin/sh
# Show that wc's new --files0-from option works.

# Copyright (C) 2006-2016 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ wc

echo 2 > 2b || framework_failure_
echo 2 words > 2w || framework_failure_
printf '2b\n2w\n' |tr '\n' '\0' > names || framework_failure_


wc --files0-from=names > out || fail=1
cat <<\EOF > exp || fail=1
 1  1  2 2b
 1  2  8 2w
 2  3 10 total
EOF

compare exp out || fail=1

if ! test "$fail" = 1; then
  # Repeat the above test, but read the file name list from stdin.
  rm -f out
  wc --files0-from=- < names > out || fail=1
  compare exp out || fail=1
fi

# Ensure file name containing new lines are output on a single line
nlname='1
2'
touch "$nlname" || framework_failure_
printf '%s\0' "$nlname" | wc --files0-from=- > out || fail=1
printf '%s\n' "0 0 0 '1'$'\\n''2'" > exp || framework_failure_
compare exp out || fail=1

Exit $fail
