#!/bin/sh
# Test that 'date' does not mishandle %% in the format string.

# Copyright (C) 2026 Free Software Foundation, Inc.

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
print_ver_ date

for loc in C "$LOCALE_FR" "$LOCALE_FR_UTF8"; do
  test -z "$loc" && continue
  # Time conversion specifiers.
  fmt=$(echo HIklMNpPrRsSTXzZ | sed 's/./%%&/g') || framework_failure_
  echo $fmt | sed 's/%%/%/g' > exp || framework_failure_
  LC_ALL="$loc" date +"$fmt" > out || fail=1
  compare exp out || fail=1
  # Date conversion specifiers.
  fmt=$(echo aAbBcCdDeFgGhjmuUVwWxyY | sed 's/./%%&/g') || framework_failure_
  echo $fmt | sed 's/%%/%/g' > exp || framework_failure_
  LC_ALL="$loc" date +"$fmt" > out || fail=1
  compare exp out || fail=1
done

Exit $fail
