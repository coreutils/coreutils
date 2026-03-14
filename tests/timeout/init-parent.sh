#!/bin/sh
# Test the behavior of 'timeout' with the init process as its parent.

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
print_ver_ timeout

newpids () { unshare --pid --fork --map-root-user "$@"; }

newpids true || skip_ 'unshare --pid --fork --map-root-user is unavailable'

# In coreutils-9.10 'timeout' would exit immediately if its parent was the init
# process.  This was discovered because Docker entrypoints have a process ID
# of 1.
newpids timeout .1 sleep 10
test $? = 124 || fail=1

Exit $fail
