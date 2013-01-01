#!/bin/sh
# Ensure that cut does not allocate mem for a range like -b9999999999999-

# Copyright (C) 2012-2013 Free Software Foundation, Inc.

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
print_ver_ cut
require_ulimit_
getlimits_

# From coreutils-8.10 through 8.20, this would make cut try to allocate
# a 256MiB bit vector.  With a 20MB limit on VM, the following would fail.
(ulimit -v 20000; : | cut -b$INT_MAX- > err 2>&1) || fail=1

compare /dev/null err || fail=1

Exit $fail
