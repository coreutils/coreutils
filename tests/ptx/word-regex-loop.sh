#!/bin/sh
# Ensure a nullable --word-regexp does not make ptx loop forever

# Copyright (C) 2026 Free Software Foundation, Inc.

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
print_ver_ ptx timeout

# A nullable word regexp matching the empty string left the cursor
# unchanged in SKIP_SOMETHING, so define_all_fields() spun forever.

echo 'aa bb cc' > in || framework_failure_

for re in 'a*' '[a-z]*' '[ab]*' '\(a\)*'; do
  timeout 10 ptx -W "$re" in >/dev/null \
    || { warn_ "ptx -W '$re' failed or hung"; fail=1; }
done

# Verify match semantics
timeout 10 ptx -W '[ab]*' in > out &&
timeout 10 ptx -W '[ab]+' in > exp &&
compare exp out || fail=1

Exit $fail
