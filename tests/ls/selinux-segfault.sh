#!/bin/sh
# Ensure we don't segfault in selinux handling

# Copyright (C) 2008-2025 Free Software Foundation, Inc.

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

# ls -l /proc/sys would segfault when built against libselinux1 2.0.15-2+b1
f=/proc/sys
test -r $f || f=.
ls -l $f > out || fail=1

# ls <= 8.32 would segfault when printing
# the security context of broken symlink targets
mkdir sedir || framework_failure_
ln -sf missing sedir/broken || framework_failure_
returns_ 1 ls -L -R -Z -m sedir > out || fail=1

# ls 9.6 would segfault with the following
ls -Z . > out || fail=1

Exit $fail
