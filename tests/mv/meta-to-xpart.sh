#!/bin/sh
# A cross-partition move of a file should maintain user and permissions

# Copyright (C) 2025-2026 Free Software Foundation, Inc.

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
require_root_

cleanup_() { rm -rf "$other_partition_tmpdir"; }
. "$abs_srcdir/tests/other-fs-tmpdir"

touch non-root-owned || framework_failure_
chown "$NON_ROOT_USERNAME" non-root-owned ||
  skip_ "can't chown to $NON_ROOT_USERNAME"
# Also check setuid bit, since it's security sensitive
# and also dependent on correct order of meta data update.
chmod u+s non-root-owned ||
  skip_ "can't set setuid bit"

mv non-root-owned "$other_partition_tmpdir" || fail=1

test -u "$other_partition_tmpdir"/non-root-owned || fail=1
test $(stat -c %U "$other_partition_tmpdir"/non-root-owned) = \
  "$NON_ROOT_USERNAME" || fail=1

Exit $fail
