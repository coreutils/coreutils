#!/bin/sh
# Verify that ls fits all entries on one row when they fit within the width.

# Copyright (C) 2026 Free Software Foundation, Inc.

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

. "${srcdir=.}/tests/init.sh";
print_ver_ ls

mkdir subdir || framework_failure_
# These 9 entries total 79 chars with 2-space separators, fitting in 96 cols.
for name in code Desktop Documents Downloads Music Pictures Public Templates Videos; do
  touch subdir/$name || framework_failure_
done

# With -w96, all entries must appear on a single line (LC_ALL=C sort order).
LC_ALL=C ls -x -w96 subdir > out || fail=1

cat <<\EOF > exp || framework_failure_
Desktop  Documents  Downloads  Music  Pictures  Public  Templates  Videos  code
EOF

compare exp out || fail=1

Exit $fail
