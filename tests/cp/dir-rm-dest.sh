#!/bin/sh
# verify that cp's --remove-destination option works with -R

# Copyright (C) 2000-2016 Free Software Foundation, Inc.

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

mkdir d e || framework_failure_

# Do it once with no destination...
cp -R --remove-destination d e || fail=1

# ...and again, with an existing destination.
cp -R --remove-destination d e || fail=1

Exit $fail
