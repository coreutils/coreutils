#!/bin/sh
# Test that 'nl' processes all arguments.

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

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ nl
getlimits_

echo a > file1 || framework_failure_
echo b > file2 || framework_failure_
returns_ 1 nl file1 missing file2 > out 2> err || fail=1
printf '     1\ta\n     2\tb\n' > exp-out || framework_failure_
cat <<EOF > exp-err || framework_failure_
nl: missing: $ENOENT
EOF
compare exp-out out || fail=1
compare exp-err err || fail=1

Exit $fail
