#!/bin/sh

# Copyright (C) 2012-2025 Free Software Foundation, Inc.

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
print_ver_ dd

echo 0123456789abcdefghijklm > in || framework_failure_

# count bytes
for operands in "count=14B" "count=14 iflag=count_bytes"; do
  dd $operands conv=swab < in > out 2> /dev/null || fail=1
  case $(cat out) in
   1032547698badc) ;;
   *) fail=1 ;;
  esac
done

for operands in "iseek=10B" "skip=10 iflag=skip_bytes"; do
  # skip bytes
  dd $operands < in > out 2> /dev/null || fail=1
  case $(cat out) in
   abcdefghijklm) ;;
   *) fail=1 ;;
  esac

  # skip records and bytes from pipe
  echo 0123456789abcdefghijklm |
    dd $operands bs=2 > out 2> /dev/null || fail=1
  case $(cat out) in
   abcdefghijklm) ;;
   *) fail=1 ;;
  esac
done

truncate -s8 expected2
printf '\0\0\0\0\0\0\0\0abcdefghijklm\n' > expected

for operands in "oseek=8B" "seek=8 oflag=seek_bytes"; do
  # seek bytes
  echo abcdefghijklm |
    dd $operands bs=5 > out 2> /dev/null || fail=1
  compare expected out || fail=1

  # Just truncation, no I/O
  dd $operands bs=5 of=out2 count=0 2> /dev/null || fail=1
  compare expected2 out2 || fail=1
done

# Check recursive integer parsing
for oseek in '1x2x4 oflag=seek_bytes' '1Bx2x4' '1Bx8' '2Bx4B' '2x4B'; do
  # seek bytes
  echo abcdefghijklm |
    dd oseek=$oseek bs=5 > out 2> /dev/null || fail=1
  compare expected out || fail=1
done

# Negative checks for integer parsing
for count in B B1 Bx1 KBB BB KBb KBx x1 1x 1xx1; do
  returns_ 1 dd count=$count </dev/null >/dev/null || fail=1
done
Exit $fail
