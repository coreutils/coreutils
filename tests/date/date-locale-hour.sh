#!/bin/sh
# Check that 'date' uses the 12-hour or 24-hour clock depending on the
# current locale.

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

locale date_fmt || skip_ "'locale date_fmt' not supported on this system"

# Fallback C locale should be supported.
for loc in "$LOCALE_FR_UTF8" 'en_US.UTF-8'; do
  case $(LC_ALL="$loc" locale date_fmt) in
    *%[Ilr]*)  compare_time='1:00' ;;
    *%[HkRT]*) compare_time='13:00' ;;
    *) skip_ 'unrecognised locale hour format';;
  esac

  case $(LC_ALL="$loc" date -d '2025-10-11T13:00') in
    *"$compare_time"*) ;;
    *) fail=1 ;;
  esac
done


Exit $fail
