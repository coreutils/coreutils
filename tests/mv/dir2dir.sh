#!/bin/sh
# Ensure that mv prints the right diagnostic for a dir->dir move
# where the destination directory is not empty.

# Copyright (C) 2006-2026 Free Software Foundation, Inc.

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
getlimits_

mkdir -p a/t b/t || framework_failure_
touch a/t/f || framework_failure_


# Expect this to fail with the expected diagnostic.
# For an interim (pre-6.0) release, it would give an erroneous
# diagnostic about moving one directory to a subdirectory of itself.
mv b/t a 2> out && fail=1

sed "s/: $EEXIST/: $ENOTEMPTY/"<out>o1;mv o1 out
sed "s/: $EBUSY/: $ENOTEMPTY/"<out>o1;mv o1 out

cat <<EOF > exp || framework_failure_
mv: cannot overwrite 'a/t': $ENOTEMPTY
EOF

compare exp out || fail=1

Exit $fail
