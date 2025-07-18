#!/bin/sh
# Verify the Thai solar calendar is used with the Thai locale.

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
print_ver_ date printf

# Current year in the Gregorian calendar.
current_year=$(LC_ALL=C date +%Y)

export LC_ALL=th_TH.UTF-8

if test "$(locale charmap 2>/dev/null)" != UTF-8; then
  skip_ "Thai UTF-8 locale not available"
fi

# Since 1941, the year in the Thai solar calendar is the Gregorian year plus
# 543.
test $(date +%Y) = $(($current_year + 543)) || fail=1

# All months that have 31 days have names that end with "คม".
days_31_suffix=$(env printf '\u0E04\u0E21')
test $(printf '%s' "$days_31_suffix" | wc -c) = 6 || skip_ 'bad suffix'
for month in 01 03 05 07 08 10 12; do
  date --date=$current_year-$month-01 +%B | grep "$days_31_suffix$" || fail=1
done

# Check that --iso-8601 and --rfc-3339 uses the Gregorian calendar.
case $(date --iso-8601=hours) in $current_year-*) ;; *) fail=1 ;; esac
case $(date --rfc-3339=date) in $current_year-*) ;; *) fail=1 ;; esac

Exit $fail
