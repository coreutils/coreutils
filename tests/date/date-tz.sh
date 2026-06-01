#!/bin/sh
# Verify TZ processing.

# Copyright (C) 2017-2026 Free Software Foundation, Inc.

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

# coreutils-8.27 would overwrite the heap with large TZ values
tz_long=$(printf '%2000s' | tr ' ' a)
date -d "TZ=\"${tz_long}0\" 2017" 2> err

# Gnulib's tzalloc handles arbitrarily long TZ values, but NetBSD's does not.
case $? in
  0) ;;
  *) grep '^date: invalid date' err || fail=1 ;;
esac

# A fixed-offset keyword (here 'UTC') anchors the instant before relative
# items are applied.  With a DST-observing zone the epoch base is in winter
# while the result lands in summer, so adding the seconds must not pick up the
# DST hour: '1970-01-01 UTC N seconds' is always exactly N seconds past the
# epoch, regardless of TZ.
secs=1780318971
TZ='Europe/Berlin' date -d "1970-01-01 UTC $secs seconds" +%s > out || fail=1
echo "$secs" > exp
compare exp out || fail=1

Exit $fail
