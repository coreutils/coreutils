#!/bin/sh
# An invalid LS_COLORS that appears after a valid "ln=target" entry
# used to make ls read the freed color buffer (heap-use-after-free).
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
print_ver_ ls

# "ln=target" stores a pointer into the color buffer,
# Until v9.12 this buffer was free'd then accessed
# after a subsequent parsing failure.
LS_COLORS='ln=target:x' ls --color=always . >/dev/null 2>err || fail=1

cat <<\EOF > exp-err || framework_failure_
ls: unparsable value for LS_COLORS environment variable
EOF

compare exp-err err || fail=1

Exit $fail
