#!/bin/sh
# Make sure that rm -r '' fails.

# Copyright (C) 1997-2016 Free Software Foundation, Inc.

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


# On SunOS 4.1.3, running rm -r '' in a nonempty directory may
# actually remove files with names of entries in the current directory
# but relative to '/' rather than relative to the current directory.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ rm

returns_ 1 rm -r '' > /dev/null 2>&1 || fail=1

Exit $fail
