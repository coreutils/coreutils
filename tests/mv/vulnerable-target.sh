#!/bin/sh
# Check that mv diagnoses vulnerable target directories.

# Copyright 2017 Free Software Foundation, Inc.

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
print_ver_ mv

unset POSIXLY_CORRECT

mkdir -m a+rwx risky || framework_failure_
mkdir risky/d || framework_failure_
echo foo >foo || framework_failure_

mv foo risky/d && fail=1
mv foo risky/d/ || fail=1
mv risky/d/foo . || fail=1
mv -t risky/d foo || fail=1
mv risky/d/foo . || fail=1
mv -T foo risky/d/foo || fail=1
mv risky/d/foo . || fail=1
POSIXLY_CORRECT=yes mv foo risky/d || fail=1
mv risky/d/foo . || fail=1

Exit $fail
