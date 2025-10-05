#!/bin/sh
# Test that cksum can guess the digest length from base64 checksums.

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
print_ver_ cksum

echo 'test input' > inp
echo 'inp: OK' > expout
echo 'cksum: truncated: no properly formatted checksum lines found' > experr

for algorithm in sha2 sha3; do
  for length in 224 256 384 512; do
    # Create files with base64 checksums in the untagged format.
    cksum -a $algorithm --length $length --base64 --untagged inp \
      > check || fail=1
    # Check that the length can be determined from the base64 checksum.
    cksum -a $algorithm --check check > out || fail=1
    compare expout out || fail=1
    # Check that only valid lengths are supported.
    sed 's|[a-zA-Z0-9+/=]  inp$|  inp|g' check > truncated || fail=1
    returns_ 1 cksum -a $algorithm --check truncated 2> err || fail=1
    compare experr err || fail=1
  done
done

for length in 8 216 224 232 248 256 264 376 384 392 504 512; do
  # Create files with base64 checksums in the untagged format.
  cksum -a blake2b --length $length --base64 --untagged inp \
    > check || fail=1
  # Check that the length can be determined from the base64 checksum.
  cksum -a blake2b --check check > out || fail=1
  compare expout out || fail=1
done

Exit $fail
