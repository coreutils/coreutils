#!/bin/sh
# Test the suffix auto widening functionality

# Copyright (C) 2012-2013 Free Software Foundation, Inc.

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
print_ver_ split


# ensure this feature is off when start number specified
truncate -s12 file.in
split file.in -b1 --numeric=89 && fail=1
test "$(ls -1 x* | wc -l)" = 11 || fail=1
rm -f x*

# ensure this feature works when no start num specified
truncate -s91 file.in
for prefix in 'x' 'xx' ''; do
    for add_suffix in '.txt' ''; do
      split file.in "$prefix" -b1 --numeric --additional-suffix="$add_suffix" \
        || fail=1
      test "$(ls -1 $prefix*[0-9]*$add_suffix | wc -l)" = 91 || fail=1
      test -e "${prefix}89$add_suffix" || fail=1
      test -e "${prefix}9000$add_suffix" || fail=1
      rm -f $prefix*[0-9]*$add_suffix
    done
done

Exit $fail
