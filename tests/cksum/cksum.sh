#!/bin/sh
# Validate cksum operation

# Copyright (C) 2020-2025 Free Software Foundation, Inc.

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


returns_ 1 cksum missing 2> /dev/null || fail=1

GLIBC_TUNABLES='glibc.cpu.hwcaps=-AVX512F,-AVX2,-AVX,-PMULL' \
 cksum --debug /dev/null 2>debug || fail=1
grep 'using.*hardware support' debug && fail=1

# Pass in expected crc and crc32b for file "in"
# Sets fail=1 upon failure
crc_check() {
  TUNABLE_DISABLE='glibc.cpu.hwcaps='
  for DHW in NONE AVX512F AVX2 AVX PMULL; do
    TUNABLE_DISABLE="$TUNABLE_DISABLE-$DHW,"
    for crct in crc crc32b; do
      GLIBC_TUNABLES="$TUNABLE_DISABLE" \
       cksum -a $crct in || fail=1
      GLIBC_TUNABLES="$TUNABLE_DISABLE" \
       cksum -a $crct in > out || fail=1
      case "$crct" in crc) crce="$1";; crc32b) crce="$2";; esac
      size=$(stat -c %s in) || framework_failure_
      printf '%s\n' "$crce $size in" > exp || framework_failure_
      compare exp out || fail=1
    done
  done
}


# Check complete range of bytes
{
  for offset in $(seq -1 6); do
    env printf $(env printf '\\%03o' $(seq 0 $offset));
    env printf $(env printf '\\%03o' $(seq 0 255));
  done
} > in || framework_failure_
crc_check 4097727897 559400337

# Make sure crc is correct for files larger than 128 bytes (4 fold pclmul)
{
  env printf $(env printf '\\%03o' $(seq 0 130));
} > in || framework_failure_
crc_check 3800919234 3739179551

# Make sure crc is correct for files larger than 32 bytes
# but <128 bytes (1 fold pclmul)
{
  env printf $(env printf '\\%03o' $(seq 0 64));
} > in || framework_failure_
crc_check 796287823 1086353368

# Make sure crc is still handled correctly when next 65k buffer is read
# (>32 bytes more than 65k)
{
  seq 1 12780
} > in || framework_failure_
crc_check 3720986905 388883562

# Make sure crc is still handled correctly when next 65k buffer is read
# (>=128 bytes more than 65k)
{
  seq 1 12795
} > in || framework_failure_
crc_check 4278270357 2796628507


Exit $fail
