#!/bin/sh
# 'cksum -a sha3' tests.

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
getlimits_

# Ensure we can --check the --tag format we produce
for i in 'a' ' b' '*c' '44' ' '; do
  echo "$i" > "$i"
  for l in 224 256 384 512; do
    cksum -a sha3 -l $l "$i" >> check.sha3
  done
done
# Note -l is inferred from the tags in the mixed format file
cksum -a sha3 --strict -c check.sha3 || fail=1

# Also ensure the openssl tagged variant works
sed 's/ //; s/ =/=/' < check.sha3 > openssl.sha3 || framework_failure_
cksum -a sha3 --strict -c openssl.sha3 || fail=1

# Ensure we can check non tagged format
for l in 224 256 384 512; do
  cksum -a sha3 --untagged --text -l $l /dev/null \
    | tee -a check.vals > check.sha3
  cksum -a sha3 -l $l --strict -c check.sha3 || fail=1
  cksum -a sha3 --strict -c check.sha3 || fail=1
done

# Ensure the checksum values are correct.  The reference
# check.vals was created using OpenSSL.
cksum -a sha3 --length=256 check.vals > out.tmp || fail=1
tr '*' ' ' < out.tmp > out || framework_failure_  # Remove binary tag on cygwin
printf '%s' 'SHA3-256 (check.vals) = ' > exp
echo 'b4753bf1696fda712821b665494c89090ffb0e87b8645559ad9f5db25b42d4f3' >> exp
compare exp out || fail=1

# Make sure --check does not handle unsupported digest sizes, e.g. truncated
# digests.
printf '%s' 'SHA3-248 (check.vals) = ' > inp
echo 'b4753bf1696fda712821b665494c89090ffb0e87b8645559ad9f5db25b42d4' >> inp
returns_ 1 cksum -a sha3 -c --warn inp 2>err || fail=1
cat <<EOF > exp || framework_failure_
cksum: inp: 1: improperly formatted SHA3 checksum line
cksum: inp: no properly formatted checksum lines found
EOF
compare exp err || fail=1

# Only validate the last specified, used length
cksum -a sha3 -l 253 -l 256 /dev/null || fail=1

# SHA-3 only allows values for --length to be 224, 256, 384, and 512.
# Check that multiples of 8 that are allowed by BLAKE2 are disallowed.
for len in 216 248 376 504 513 1024 $UINTMAX_OFLOW; do
  returns_ 1 cksum -a sha3 -l $len /dev/null 2>err || fail=1
  cat <<EOF > exp || framework_failure_
cksum: invalid length: '$len'
cksum: digest length for 'SHA3' must be 224, 256, 384, or 512
EOF
  compare exp err || fail=1
  # We still check --length when --check is used.
  returns_ 1 cksum -a sha3 -l $len --check /dev/null 2>err || fail=1
  compare exp err
done

Exit $fail
