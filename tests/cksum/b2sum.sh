#!/bin/sh
# 'b2sum' tests

# Copyright (C) 2016-2025 Free Software Foundation, Inc.

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

for prog in 'b2sum' 'cksum -a blake2b'; do
# Also check b2sum if built
[ "$prog" = 'b2sum' ] && { $prog --version || continue; }

# Ensure we can --check the --tag format we produce
rm -f check.b2sum || framework_failure_
[ "$prog" = 'b2sum' ] && tag_opt='--tag' || tag_opt=''
for i in 'a' ' b' '*c' '44' ' '; do
  echo "$i" > "$i"
  for l in 0 128; do
    $prog -l $l $tag_opt "$i" >> check.b2sum
  done
done
# Note -l is inferred from the tags in the mixed format file
$prog --strict -c check.b2sum || fail=1
# Also ensure the openssl tagged variant works
sed 's/ //; s/ =/=/' < check.b2sum > openssl.b2sum || framework_failure_
$prog --strict -c openssl.b2sum || fail=1

rm -f check.vals || framework_failure_
# Ensure we can check non tagged format
[ "$prog" != 'b2sum' ] && tag_opt='--untagged' || tag_opt=''
for l in 0 128; do
  $prog $tag_opt --text -l $l /dev/null | tee -a check.vals > check.b2sum
  $prog -l $l --strict -c check.b2sum || fail=1
  $prog --strict -c check.b2sum || fail=1
done

# Ensure the checksum values are correct.  The reference
# check.vals was created with the upstream SSE reference implementation.
[ "$prog" != 'b2sum' ] && tag_opt='--untagged' || tag_opt=''
$prog $tag_opt --length=128 check.vals > out.tmp || fail=1
tr '*' ' ' < out.tmp > out || framework_failure_  # Remove binary tag on cygwin
printf '%s\n' '796485dd32fe9b754ea5fd6c721271d9  check.vals' > exp
compare exp out || fail=1

# This would segfault from coreutils-8.26 to coreutils-8.28
printf '%s\n' 'BLAKE2' 'BLAKE2b' 'BLAKE2-' 'BLAKE2(' 'BLAKE2 (' > crash.check \
  || framework_failure_
returns_ 1 $prog -c crash.check || fail=1

# This would read unallocated memory from coreutils-9.2 to coreutils-9.3
# which would trigger with ASAN or valgrind
printf '0A0BA0' > overflow.check || framework_failure_
returns_ 1 $prog -c overflow.check || fail=1

# This would fail before coreutil-9.4
# Only validate the last specified, used length
$prog -l 123 -l 128 /dev/null || fail=1

# This would not flag an error in coreutils 9.6 and 9.7
for len in 513 1024 $UINTMAX_OFLOW; do
  returns_ 1 $prog -l $len /dev/null 2>err || fail=1
  progname=$(echo "$prog" | cut -f1 -d' ')
  cat <<EOF > exp || framework_failure_
$progname: invalid length: '$len'
$progname: maximum digest length for 'BLAKE2b' is 512 bits
EOF
  compare exp err || fail=1
done

done

Exit $fail
