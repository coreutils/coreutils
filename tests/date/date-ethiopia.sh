#!/bin/sh
# Verify the Ethiopian calendar is used in the Ethiopian locale.

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

export LC_ALL=am_ET.UTF-8

if test "$(locale charmap 2>/dev/null)" != UTF-8; then
  skip_ "Ethiopian UTF-8 locale not available"
fi

# 09-10 and 09-12 of the same Gregorian year are in different years in the
# Ethiopian calendar.
year_september_10=$(date -d $current_year-09-10 +%Y)
year_september_12=$(date -d $current_year-09-12 +%Y)
test $year_september_10 = $(($year_september_12 - 1)) || fail=1

# The difference between the Gregorian year is 7 or 8 years.
test $year_september_10 = $(($current_year - 8)) || fail=1
test $year_september_12 = $(($current_year - 7)) || fail=1

# Check that --iso-8601 and --rfc-3339 uses the Gregorian calendar.
case $(date --iso-8601=hours) in $current_year-*) ;; *) fail=1 ;; esac
case $(date --rfc-3339=date) in $current_year-*) ;; *) fail=1 ;; esac

Exit $fail
