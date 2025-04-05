#!/bin/sh
# Validate sleep parameters

# Copyright (C) 2016-2025 Free Software Foundation, Inc.

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
print_ver_ sleep printf
getlimits_

# invalid timeouts
returns_ 1 timeout 10 sleep invalid || fail=1
returns_ 1 timeout 10 sleep -- -1 || fail=1
returns_ 1 timeout 10 sleep 42D || fail=1
returns_ 1 timeout 10 sleep 42d 42day || fail=1
returns_ 1 timeout 10 sleep nan || fail=1
returns_ 1 timeout 10 sleep '' || fail=1
returns_ 1 timeout 10 sleep || fail=1

# subsecond actual sleep
timeout 10 sleep 0.001 || fail=1
timeout 10 sleep 0x.002p1 || fail=1
timeout 10 sleep 0x0.01d || fail=1  # d is part of hex, not a day suffix

# Using small timeouts for larger sleeps is racy,
# but false positives should be avoided on most systems
returns_ 124 timeout 0.1 sleep 1d 2h 3m 4s || fail=1
returns_ 124 timeout 0.1 sleep inf || fail=1
returns_ 124 timeout 0.1 sleep $LDBL_MAX || fail=1

# Test locale decimal handling for printf, sleep, timeout
if test "$LOCALE_FR_UTF8" != "none"; then
  f=$LOCALE_FR_UTF8
  locale_decimal=$(LC_ALL=$f env printf '%0.3f' 0.001) || fail=1
  locale_decimal=$(LC_ALL=$f env printf '%0.3f' "$locale_decimal") || fail=1
  case "$locale_decimal" in
    0?001)
      LC_ALL=$f timeout 1$locale_decimal sleep "$locale_decimal" || fail=1 ;;
    *) fail=1 ;;
  esac
fi

Exit $fail
