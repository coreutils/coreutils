#!/bin/sh
# Make sure cp --attributes-only doesn't truncate existing data

# Copyright 2012-2025 Free Software Foundation, Inc.

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
print_ver_ cp

printf '1' > file1 || framework_failure_
printf '2' > file2 || framework_failure_
printf '2' > file2.exp || framework_failure_

cp --attributes-only file1 file2 || fail=1
cmp file2 file2.exp || fail=1

# coreutils v8.32 and before would remove destination files
# if hardlinked or the source was not a regular file.
ln file2 link2 || framework_failure_
cp -a --attributes-only file1 file2 || fail=1
cmp file2 file2.exp || fail=1

ln -s file1 sym1 || framework_failure_
returns_ 1 cp -a --attributes-only sym1 file2 || fail=1
cmp file2 file2.exp || fail=1

# One can still force removal though
cp -a --remove-destination --attributes-only sym1 file2 || fail=1
test -L file2 || fail=1
cmp file1 file2 || fail=1

Exit $fail
