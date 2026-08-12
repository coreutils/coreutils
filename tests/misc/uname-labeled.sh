#!/bin/sh
# Test -A / --all-labeled option of uname

# Copyright (C) 2026 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ uname

# Test basic -A / --all-labeled functionality
uname -A > out || fail=1
uname --all-labeled > out_long || fail=1
compare out out_long || fail=1

# Check that the output format has labels and values
grep '^Kernel name: ' out || fail=1
grep '^Node name: ' out || fail=1
grep '^Kernel release: ' out || fail=1
grep '^Kernel version: ' out || fail=1
grep '^Machine: ' out || fail=1
# processor and hardware-platform may be omitted if unknown
grep '^Operating system: ' out || fail=1

# Ensure that standard uname output (e.g. uname -a) is unchanged
uname -a > out_std || fail=1
# out_std must be a single line and not contain the labels
test $(wc -l < out_std) -eq 1 || fail=1
grep -F 'Kernel name:' out_std && fail=1
grep -F 'Node name:' out_std && fail=1
grep -F 'Kernel release:' out_std && fail=1

Exit $fail
