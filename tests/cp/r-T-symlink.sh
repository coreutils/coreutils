#!/bin/sh
# Ensure cp -rT doesn't copy through a destination symlink to a directory.

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
print_ver_ cp

mkdir src dir || framework_failure_
touch src/file || framework_failure_
ln -s dir dst || framework_failure_

# With -T, the symlink itself is the destination and is not replaced.
returns_ 1 cp -rT src dst 2>err || fail=1
cat <<\EOF2 > exp || framework_failure_
cp: cannot overwrite non-directory 'dst' with directory 'src'
EOF2
compare exp err || fail=1
test -h dst || fail=1
test -e dir/file && fail=1

# With a trailing slash the symlink is followed.
cp -rT src dst/ || fail=1
test -f dir/file || fail=1
rm dir/file || framework_failure_

# Without -T, the copy goes into the directory the symlink resolves to.
cp -r src/. dst || fail=1
test -f dir/file || fail=1
cp -r src dst || fail=1
test -f dir/src/file || fail=1
test -h dst || fail=1

Exit $fail
