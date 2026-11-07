#!/bin/sh
# Test large inputs to basenc

# Copyright (C) 2025-2026 Free Software Foundation, Inc.

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
print_ver_ basenc

# coreutils v9.8 would not operate correctly with > 15,561,475 bytes
truncate -s20MiB file.zeros || framework_failure_

test $(basenc --base58 file.zeros | wc -c) = 21247462 || fail=1

# Ensure base58 input of more than 1024 digits is decoded as one number.
seq 300 > nums || framework_failure_
basenc --base58 -w0 nums > nums.b58 || fail=1
test $(wc -c < nums.b58) -gt 1024 || framework_failure_
basenc --base58 -d nums.b58 > back || fail=1
compare nums back || fail=1

Exit $fail
