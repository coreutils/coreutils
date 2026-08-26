#!/bin/sh
# Exercise format strings involving %:X, %:Y, etc.

# Copyright (C) 2010-2026 Free Software Foundation, Inc.

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
print_ver_ stat

# Set this to avoid problems with weird time zones.
TZ=UTC0
export TZ

# Use a timestamp near the Epoch to avoid trouble with leap seconds.
touch -d '1970-01-01 18:43:33.023456789' k || framework_failure_

ls --full-time | grep 18:43:33.023456789 \
  || skip_ this file system does not support sub-second timestamps

test "$(stat -c       %Y k)" =    67413               || fail=1
test "$(stat -c      %.Y k)" =    67413.023456789     || fail=1
test "$(stat -c     %.1Y k)" =    67413.0             || fail=1
test "$(stat -c     %.3Y k)" =    67413.023           || fail=1
test "$(stat -c     %.6Y k)" =    67413.023456        || fail=1
test "$(stat -c     %.9Y k)" =    67413.023456789     || fail=1
test "$(stat -c   %13.6Y k)" =  ' 67413.023456'       || fail=1
test "$(stat -c  %013.6Y k)" =   067413.023456        || fail=1
test "$(stat -c  %-13.6Y k)" =   '67413.023456 '      || fail=1
test "$(stat -c  %18.10Y k)" = '  67413.0234567890'   || fail=1
test "$(stat -c %I18.10Y k)" = '  67413.0234567890'   || fail=1
test "$(stat -c %018.10Y k)" =  0067413.0234567890    || fail=1
test "$(stat -c %-18.10Y k)" =   '67413.0234567890  ' || fail=1

# A recent timestamp needs more than the 53 bits of a double to hold
# all nine fractional digits.
# Use the epoch notation for '2026-05-04 03:02:01.987654321'
# to avoid any leap second ambiguities.
touch -d '@1777863721.987654321' recent || framework_failure_

test "$(stat -c   %Y recent)" = 1777863721            || fail=1
test "$(stat -c  %.Y recent)" = 1777863721.987654321  || fail=1
test "$(stat -c %.3Y recent)" = 1777863721.987        || fail=1
test "$(stat -c %.9Y recent)" = 1777863721.987654321  || fail=1

# Timestamps before the Epoch truncate towards minus infinity.
# Use epoch notation for '1969-12-31 23:59:59.123456789'
touch -d '@-0.876543211' old || framework_failure_

# If touch before the epoch wasn't ignored,
# and wasn't clamped to the epoch.
# E.g. FreeBSD uses tv_sec == -1 to indicate that vnode
# should not have it's timestamp adjusted.
if test $(stat -c %Y old) -lt 1 &&
   test $(stat -c %.1Y old) != '0.0'; then
  test "$(stat -c   %Y old)" = -1            || fail=1
  test "$(stat -c  %.Y old)" = -0.876543211  || fail=1
  test "$(stat -c %.3Y old)" = -0.876        || fail=1
  test "$(stat -c %.9Y old)" = -0.876543211  || fail=1
fi

Exit $fail
