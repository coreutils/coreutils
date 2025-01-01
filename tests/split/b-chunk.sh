#!/bin/sh
# test splitting into 3 chunks

# Copyright (C) 2010-2025 Free Software Foundation, Inc.

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
print_ver_ split

# N can be greater than the file size
# in which case no data is extracted, or empty files are written
split -n 10 /dev/null || fail=1
test "$(stat -c %s x* | uniq -c | sed 's/^ *//; s/ /x/')" = "10x0" || fail=1
rm -f x??

printf 'abc' > abc || framework_failure_
printf 'a' > exp-a || framework_failure_
printf 'b' > exp-b || framework_failure_
printf 'c' > exp-c || framework_failure_
printf 'ab' > exp-ab || framework_failure_
split -n 4 abc || fail=1
compare exp-a xaa || fail=1
compare exp-b xab || fail=1
compare exp-c xac || fail=1
compare /dev/null xad || fail=1
test ! -f xae || fail=1
rm -f x??
split -n 2 abc || fail=1
compare exp-ab xaa || fail=1
compare exp-c xab || fail=1
test ! -f xac || fail=1
rm -f x??

# When extracting K of N where N > file size
# no data is extracted, and no files are written
split -n 2/3 /dev/null || fail=1
returns_ 1 stat x?? 2>/dev/null || fail=1

# Ensure --elide-empty-files is honored
split -e -n 10 /dev/null || fail=1
returns_ 1 stat x?? 2>/dev/null || fail=1

printf '1\n2\n3\n4\n5\n' > input || framework_failure_
printf '1\n2\n' > exp-1 || framework_failure_
printf '3\n4' > exp-2 || framework_failure_
printf '\n5\n' > exp-3 || framework_failure_

for file in input /proc/version /sys/kernel/profiling; do
  test -f $file || continue

  for blksize in 1 2 4096; do
    if ! test "$file" = 'input'; then
      # For /proc like files we must be able to read all
      # into the internal buffer to be able to determine size.
      test "$blksize" = 4096 || continue
    fi

    split -n 3 ---io-blksize=$blksize $file > out || fail=1
    split -n 1/3 ---io-blksize=$blksize $file > b1 || fail=1
    split -n 2/3 ---io-blksize=$blksize $file > b2 || fail=1
    split -n 3/3 ---io-blksize=$blksize $file > b3 || fail=1

    case $file in
      input)
        compare exp-1 xaa || fail=1
        compare exp-2 xab || fail=1
        compare exp-3 xac || fail=1
        ;;
    esac

    compare xaa b1 || fail=1
    compare xab b2 || fail=1
    compare xac b3 || fail=1
    cat xaa xab xac | compare - $file || fail=1
    test -f xad && fail=1
  done
done

Exit $fail
