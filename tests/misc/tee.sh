#!/bin/sh
# test for basic tee functionality.

# Copyright (C) 2005-2015 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ tee

echo line >sample || framework_failure_
nums=$(seq 9) || framework_failure_

for n in 0 $nums; do
        files=$(seq $n)
        rm -f $files
        tee $files <sample >out || fail=1
        for f in out $files; do
                compare sample $f || fail=1
        done
done


# Ensure tee exits early if no more writable outputs
if test -w /dev/full && test -c /dev/full; then
  yes | returns_ 1 timeout 10 tee /dev/full 2>err >/dev/full || fail=1
  # Ensure an error for each of the 2 outputs
  # (and no redundant errors for stdout).
  test $(wc -l < err) = 2 || { cat err; fail=1; }


  # Ensure we continue with outputs that are OK
  seq 10000 > multi_read || framework_failure_

  returns_ 1 tee /dev/full out2 2>err >out1 <multi_read || fail=1
  cmp multi_read out1 || fail=1
  cmp multi_read out2 || fail=1
  # Ensure an error for failing output
  test $(wc -l < err) = 1 || { cat err; fail=1; }

  returns_ 1 tee out1 out2 2>err >/dev/full <multi_read || fail=1
  cmp multi_read out1 || fail=1
  cmp multi_read out2 || fail=1
  # Ensure an error for failing output
  test $(wc -l < err) = 1 || { cat err; fail=1; }
fi

Exit $fail
