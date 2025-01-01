#!/bin/sh
# Test --{hex,numeric}-suffixes[=from]

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
print_ver_ split

printf '1\n2\n3\n4\n5\n' > in || framework_failure_

printf '%s\n' 1 2 > exp-0 || framework_failure_
printf '%s\n' 3 4 > exp-1 || framework_failure_
printf '%s\n' 5   > exp-2 || framework_failure_

for mode in 'numeric' 'hex'; do

  for start in 0 9; do
    mode_option="--$mode-suffixes"
    # check with and without specified start value
    test $start != '0' && mode_option="$mode_option=$start"
    split $mode_option --lines=2 in || fail=1

    test $mode = 'hex' && format=x || format=d
    for i in $(seq $start $(($start+2))); do
      compare exp-$(($i-$start)) x$(printf %02$format $i) || fail=1
    done
  done

  # Check that split failed when suffix length is not large enough for
  # the numerical suffix start value
  returns_ 1 split -a 3 --$mode-suffixes=1000 in 2>/dev/null || fail=1

  # check invalid --$mode-suffixes start values are flagged
  returns_ 1 split --$mode-suffixes=-1 in 2> /dev/null || fail=1
  returns_ 1 split --$mode-suffixes=one in 2> /dev/null || fail=1
done

Exit $fail
