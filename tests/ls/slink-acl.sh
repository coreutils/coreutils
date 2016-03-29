#!/bin/sh
# verify that ls -lL works when applied to a symlink to an ACL'd file

# Copyright (C) 2011-2016 Free Software Foundation, Inc.

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
print_ver_ ls

require_setfacl_

touch k || framework_failure_
setfacl -m user::r-- k || framework_failure_
ln -s k s || framework_failure_

set _ $(ls -Log s); shift; link=$1
set _ $(ls -og k);  shift; reg=$1

test "$link" = "$reg" || fail=1

Exit $fail
