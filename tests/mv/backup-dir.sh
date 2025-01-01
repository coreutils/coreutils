#!/bin/sh
# Ensure "mv --verbose --backup" works the same for dirs and non-dirs.

# Copyright (C) 2006-2025 Free Software Foundation, Inc.

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

mkdir A B || framework_failure_
touch X Y || framework_failure_


# Before coreutils-6.2, the " (backup: 'B.~1~')" suffix was not printed.
mv --verbose --backup=numbered -T A B > out || fail=1
cat <<\EOF > exp || framework_failure_
renamed 'A' -> 'B' (backup: 'B.~1~')
EOF

compare exp out || fail=1

# Bug#52410
mkdir C D E || framework_failure_
mv -T --backup=numbered C E/ || fail=1
mv -T --backup=numbered D E/ || fail=1

# Bug#55029
mkdir F && echo 1 >1 && echo 2 >2 && cp 1 F/X && cp 2 X || framework_failure_
mv --backup=simple X F/ || fail=1
compare 1 F/X~ || fail=1
compare 2 F/X || fail=1

Exit $fail
