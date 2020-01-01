#!/bin/sh
# Verify TZ processing.

# Copyright (C) 2017-2020 Free Software Foundation, Inc.

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
date -d "TZ=\"${tz_long}0\" 2017" || fail=1

Exit $fail
