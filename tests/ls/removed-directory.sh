#!/bin/sh
# If ls is asked to list a removed directory (e.g. the parent process's
# current working directory that has been removed by another process), it
# emits an error message.

# Copyright (C) 2020 Free Software Foundation, Inc.

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
print_ver_ ls

case $host_triplet in
  *linux*) ;;
  *) skip_ 'non linux kernel' ;;
esac

LS_FAILURE=2

cat <<\EOF >exp-err || framework_failure_
ls: reading directory '.': No such file or directory
EOF

cwd=$(pwd)
mkdir d || framework_failure_
cd d || framework_failure_
rmdir ../d || framework_failure_

returns_ $LS_FAILURE ls >../out 2>../err || fail=1
cd "$cwd" || framework_failure_
compare /dev/null out || fail=1
compare exp-err err || fail=1

Exit $fail
