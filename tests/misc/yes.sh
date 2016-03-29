#!/bin/sh
# Validate yes buffer handling

# Copyright (C) 2015-2016 Free Software Foundation, Inc.

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
print_ver_ yes

# Check various single item sizes, with the most important
# size being BUFSIZ used for the local buffer to yes(1).
# Note a \n is added, so actual sizes required internally
# are 1 more than the size used here.
for size in 1 1999 4095 4096 8191 8192 16383 16384; do
  printf "%${size}s\n" '' > out.1
  yes "$(printf %${size}s '')" | head -n2 | uniq > out.2
  compare out.1 out.2 || fail=1
done

# Check the many small items case,
# both fitting and overflowing the internal buffer
external=env
if external true $(seq 4000); then
  for i in 100 4000; do
    seq $i | paste -s -d ' ' | sed p > out.1
    yes $(seq $i) | head -n2 > out.2
    compare out.1 out.2 || fail=1
  done
fi

Exit $fail
