#!/bin/sh
# Verify the Solar Hijri calendar is used in the Iranian locale.

# Copyright (C) 2025 Free Software Foundation, Inc.

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

# Current year in the Gregorian calendar.
current_year=$(LC_ALL=C date +%Y)

export LC_ALL=fa_IR.UTF-8

if test "$(locale charmap 2>/dev/null)" != UTF-8; then
  skip_ "Iranian UTF-8 locale not available"
fi

# 03-19 and 03-22 of the same Gregorian year are in different years in the
# Solar Hijri calendar.
year_march_19=$(date -d $current_year-03-19 +%Y)
year_march_22=$(date -d $current_year-03-22 +%Y)
test $year_march_19 = $(($year_march_22 - 1)) || fail=1

# The difference between the Gregorian year is 621 or 622 years.
test $year_march_19 = $(($current_year - 622)) || fail=1
test $year_march_22 = $(($current_year - 621)) || fail=1

# Check that --iso-8601 and --rfc-3339 uses the Gregorian calendar.
case $(date --iso-8601=hours) in $current_year-*) ;; *) fail=1 ;; esac
case $(date --rfc-3339=date) in $current_year-*) ;; *) fail=1 ;; esac

Exit $fail
