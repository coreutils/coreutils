#!/bin/sh
# Ensure no loop when a numbered backup exists in an unreadable directory.

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
skip_if_root_
getlimits_

mkdir d &&
echo original > original &&
echo new > new &&
cp original d/file &&
chmod a-r d || framework_failure_
test -r d && skip_ 'cannot make directory unreadable'

# A backup can still be made if the guessed name is unused.
timeout 10 cp --backup=numbered new d/file || fail=1

# But rescanning cannot find another name
returns_ 1 timeout 10 cp --backup=numbered original d/file 2>err || fail=1
chmod u+r d || framework_failure_
echo "cp: cannot backup 'd/file': $EEXIST" >exp || framework_failure_
compare exp err || fail=1
compare new d/file || fail=1
compare original d/file.~1~ || fail=1

Exit $fail
