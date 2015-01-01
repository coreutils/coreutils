#!/bin/sh
# Ensure cp warns about but otherwise ignores source
# items specified multiple times.

# Copyright (C) 2014-2015 Free Software Foundation, Inc.

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
print_ver_ cp

mkdir a || framework_failure_
touch f || framework_failure_

# verify multiple files and dir sources only warned about
mkdir dest || framework_failure_
cp -a a a f f dest 2>err || fail=1
rm -Rf dest || framework_failure_

# verify multiple dirs and files with different names copied
mkdir dest || framework_failure_
ln -s a al || framework_failure_
ln -s f fl || framework_failure_
cp -aH a al f fl dest 2>>err || fail=1

cat <<EOF >exp
cp: warning: source directory 'a' specified more than once
cp: warning: source file 'f' specified more than once
EOF

compare exp err || fail=1

Exit $fail
