#!/bin/sh
# Validate cksum --raw operation

# Copyright (C) 2023-2025 Free Software Foundation, Inc.

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
print_ver_ cksum date od

cat > digest_types <<\EOF || framework_failure_
bsd     u2
sysv    u2
crc     u4
md5     x1
sha1    x1
sha2    x1  -l224 -l256 -l384 -l512
sha3    x1  -l224 -l256 -l384 -l512
blake2b x1  -l8 -l256 -l512
sm3     x1
EOF

date > file.in || framework_failure_

while read algo type lengths; do
  : "${lengths:=-l0}"
  for len in $lengths; do
    # Binary converted back to text
    cksum --raw -a $algo $len file.in > digest.bin || fail=1
    d='digest.bin.txt'
    od --endian=big -An -w1024 -t$type < digest.bin | tr -d ' ' \
      > "$d" || framework_failure_
    # Pad the bsd checksum with leading 0's, if needed.
    case $algo in bsd) n=$(cat "$d"); printf '%05d\n' "$n" > "$d" ;; esac

    # Standard text output
    cksum --untagged -a $algo $len < file.in | cut -d ' ' -f1 \
      > digest.txt || fail=1

    compare digest.txt "$d" || fail=1
  done
done < digest_types

# Ensure --base64 and --raw not used together
returns_ 1 cksum --base64 --raw </dev/null || fail=1

# Ensure --raw not supported with multiple files
returns_ 1 cksum --raw /dev/null /dev/null || fail=1

Exit $fail
