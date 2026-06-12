#!/bin/sh
# Test 'pr' option handling

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
print_ver_ pr
getlimits_

# Ensure pr treats all invalid +page ranges as a file
for p in +0 +0foo; do
  returns_ 1 pr "$p" 2>err </dev/null
  printf '%s\n' "pr: $p: $ENOENT" >exp || framework_failure_
  compare exp err || fail=1
done

# number parsing issue
returns_ 1 pr --pages=-0 2>err </dev/null
printf '%s\n' "pr: invalid --pages argument '-0'" >exp
compare exp err || fail=1

# number validation issue
returns_ 1 pr --pages=0 2>err </dev/null
printf '%s\n' "pr: invalid page range '0'" >exp
compare exp err || fail=1

Exit $fail
