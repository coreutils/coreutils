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

# If TZ database available (Belize doesn't have DST)
if test "$(TZ=America/Belize date +%z)" = '-0600'; then

  # A nonexistent local time in the spring-forward gap is rejected.
  returns_ 1 env \
   TZ=America/New_York date -d '2024-03-10 02:30' +%T 2>err || fail=1
  printf "date: invalid date '2024-03-10 02:30'\n" > exp || framework_failure_
  compare exp err || fail=1

  # An ambiguous local time in the fall-back overlap takes the earlier,
  # still-DST offset (+0100 here, not +0200).
  TZ=Europe/Paris date -d '2024-10-27 02:30:00' '+%Y-%m-%dT%T%z' > out || fail=1
  printf "2024-10-27T02:30:00+0100\n" > exp || framework_failure_
  compare exp out || fail=1

fi

Exit $fail
