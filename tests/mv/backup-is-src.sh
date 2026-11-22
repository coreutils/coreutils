#!/bin/sh
# Force mv to use the copying code.

# Copyright (C) 1998-2026 Free Software Foundation, Inc.

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
print_ver_ mv
cleanup_() { rm -rf "$other_partition_tmpdir"; }
. "$abs_srcdir/tests/other-fs-tmpdir"

a="$other_partition_tmpdir/a"
a2="$other_partition_tmpdir/a~"

echo a >exp-a &&
echo a2 >exp-a2 &&
cp exp-a "$a" &&
cp exp-a2 "$a2" &&
cat >exp <<EOF || framework_failure_
mv: backing up '$a' might destroy source;  '$a2' not moved
EOF

# This mv command should exit nonzero.
returns_ 1 mv --b=simple "$a2" "$a" >out 2>err || fail=1
compare /dev/null out || fail=1
compare exp err || fail=1
compare exp-a "$a" || fail=1
compare exp-a2 "$a2" || fail=1

Exit $fail
