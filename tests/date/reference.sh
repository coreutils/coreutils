#!/bin/sh
# Test date --reference

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

# Avoid any possible glitches due to daylight-saving changes near the
# timestamps used during the test.
TZ=UTC0
export TZ

t1='2025-10-23 03:00'
t2='2025-10-23 04:00'

# date(1) only considers modiication time
touch -m -d "$t1" a || framework_failure_
touch -m -d "$t2" b || framework_failure_

test $(date +%s -r a) -lt $(date +%s -r b) || fail=1

if test "$(date +%s -d "$t1")" -lt "$(date +%s)"; then
  test "$(date +%s -r a)" -lt "$(date +%s)" || fail=1
fi

ln -s t1 t1s || framework_failure_
if ! test t1 -ot t1s && ! test t1 -nt t1s; then
  test "$(date -r t1)" = "$(date -r t1s)" || fail=1
fi

returns_ 1 date --reference || fail=1
returns_ 1 date --reference= || fail=1
returns_ 1 date --reference=missing || fail=1
returns_ 1 date -d "$t1" -r/ || fail=1

Exit $fail
