#!/bin/sh
# Test date resolution interface

# Copyright (C) 2025 Free Software Foundation, Inc.

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
print_ver_ date

# Ensure --resolution is supported (prints to full nano second resolution)
res=$(date --resolution) || fail=1

# Ensure %-N format is supported (prints to most concise resolution)
subsec1=$(printf '%s\n' "$res" | sed 's/.*\.//; s/0*$//' | wc -c) ||
  framework_failure_
subsec2=$(date +%-N | wc -c) || framework_failure_
test "$subsec1" = "$subsec2" || fail=1


Exit $fail
