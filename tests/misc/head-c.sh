#!/bin/sh
# exercise head -c

# Copyright (C) 2001-2013 Free Software Foundation, Inc.

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
print_ver_ head
require_ulimit_v_
getlimits_

# exercise the fix of 2001-08-18, based on test case from Ian Bruce
echo abc > in || framework_failure_
(head -c1; head -c1) < in > out || fail=1
case "$(cat out)" in
  ab) ;;
  *) fail=1 ;;
esac

# Only allocate memory as needed.
# Coreutils <= 8.21 would allocate memory up front
# based on the value passed to -c
(ulimit -v 20000; head --bytes=-$SSIZE_MAX < /dev/null) || fail=1

Exit $fail
