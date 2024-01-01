#!/bin/sh
# Validate cksum operation

# Copyright (C) 2020-2024 Free Software Foundation, Inc.

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
print_ver_ cksum printf

returns_ 1 cksum missing || fail=1

{
  for offset in $(seq -1 6); do
    env printf $(env printf '\\%03o' $(seq 0 $offset));
    env printf $(env printf '\\%03o' $(seq 0 255));
  done
} > in || framework_failure_

cksum in > out || fail=1
printf '%s\n' '4097727897 2077 in' > exp || framework_failure_
compare exp out || fail=1

# Make sure crc is correct for files larger than 128 bytes (4 fold pclmul)
{
  env printf $(env printf '\\%03o' $(seq 0 130));
} > in || framework_failure_

cksum in > out || fail=1
printf '%s\n' '3800919234 131 in' > exp || framework_failure_
compare exp out || fail=1

# Make sure crc is correct for files larger than 32 bytes
# but <128 bytes (1 fold pclmul)
{
  env printf $(env printf '\\%03o' $(seq 0 64));
} > in || framework_failure_

cksum in > out || fail=1
printf '%s\n' '796287823 65 in' > exp || framework_failure_
compare exp out || fail=1

# Make sure crc is still handled correctly when next 65k buffer is read
# (>32 bytes more than 65k)
{
  seq 1 12780
} > in || framework_failure_

cksum in > out || fail=1
printf '%s\n' '3720986905 65574 in' > exp || framework_failure_
compare exp out || fail=1

# Make sure crc is still handled correctly when next 65k buffer is read
# (>=128 bytes more than 65k)
{
  seq 1 12795
} > in || framework_failure_

cksum in > out || fail=1
printf '%s\n' '4278270357 65664 in' > exp || framework_failure_
compare exp out || fail=1

Exit $fail
