#!/bin/sh
# Verify that 'od -N N' reads no more than N bytes of input.

# Copyright (C) 2001-2025 Free Software Foundation, Inc.

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
print_ver_ od

echo abcdefg > in || framework_failure_
(od -An -N3 -c; od -An -N3 -c) < in > out
cat <<EOF > exp || framework_failure_
   a   b   c
   d   e   f
EOF
compare exp out || fail=1

# coreutils <= 9.7 would buffer overflow with
# a single NUL byte after the heap buffer
printf '%100s' | od -N100 -S1 > out || fail=1
printf '%07o %100s\n' 0 '' > exp || framework_failure_
compare exp out || fail=1

# coreutils <= 9.7 would output nothing
printf '%100s' | od -N10 -S10 > out || fail=1
printf '%07o %10s\n' 0 '' > exp || framework_failure_
compare exp out || fail=1

# coreutils <= 9.7 would output an invalid address
printf '%100s' | od -N10 -S1 > out || fail=1
printf '%07o %10s\n' 0 '' > exp || framework_failure_
compare exp out || fail=1

# Ensure -S limits appropriately
printf '%10s\000' | od -N11 -S11 > out || fail=1
compare /dev/null out || fail=1
printf '%10s\000' | od -S11 > out || fail=1
compare /dev/null out || fail=1
printf '%10s' | od -S10 > out || fail=1 # Ignore unterminated at EOF?
compare /dev/null out || fail=1
printf '\001%10s\000%10s\000' | od -S10 > out || fail=1
printf '%07o %10s\n' 1 '' 12 '' > exp || framework_failure_
compare exp out || fail=1

Exit $fail
