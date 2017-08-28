#!/bin/sh
# Ensure that runcon does not reorder its arguments.

# Copyright (C) 2017 Free Software Foundation, Inc.

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
print_ver_ runcon

cat <<\EOF >inject.py || framework_failure_
import fcntl, termios
fcntl.ioctl(0, termios.TIOCSTI, '\n')
EOF

python inject.py || skip_ 'python TIOCSTI check failed'

returns_ 1 runcon $(id -Z) python inject.py || fail=1

Exit $fail
