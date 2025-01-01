#!/bin/sh
# Test mv --no-copy.

# Copyright (C) 2023-2025 Free Software Foundation, Inc.

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

mkdir dir || framework_failure_
> dir/a || framework_failure_
> file || framework_failure_

returns_ 1 mv --no-copy dir "$other_partition_tmpdir" || fail=1
returns_ 1 mv --no-copy file "$other_partition_tmpdir" || fail=1
mv dir "$other_partition_tmpdir" || fail=1
mv file "$other_partition_tmpdir" || fail=1

Exit $fail
