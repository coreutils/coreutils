#!/bin/sh
# Show that wc's --total option works.

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
print_ver_ wc
require_sparse_support_ # for 'truncate --size=$BIG'
getlimits_

printf '%s\n' '2' > 2b || framework_failure_
printf '%s\n' '2 words' > 2w || framework_failure_

returns_ 1 wc --total 2b 2w > out || fail=1

wc --total=never 2b 2w > out || fail=1
cat <<\EOF > exp || framework_failure_
 1  1  2 2b
 1  2  8 2w
EOF
compare exp out || fail=1

wc --total=only 2b 2w > out || fail=1
cat <<\EOF > exp || framework_failure_
2 3 10
EOF
compare exp out || fail=1

wc --total=always 2b > out || fail=1
test "$(wc -l < out)" = 2 || fail=1

# Skip this test on GNU/Hurd since it will exhaust memory there.
if test "$(uname)" != GNU && truncate -s 2E big; then
  if test "$UINTMAX_MAX" = '18446744073709551615'; then
    # Ensure overflow is diagnosed
    returns_ 1 wc --total=only -c big big big big big big big big \
      > out || fail=1

    # Ensure total is saturated
    printf '%s\n' "$UINTMAX_MAX" > exp || framework_failure_
    compare exp out || fail=1

    # Ensure overflow is ignored if totals not shown
    wc --total=never -c big big big big big big big big || fail=1
  fi
fi

Exit $fail
