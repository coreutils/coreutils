#!/bin/sh
# Test 'pr' numeric option handling

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
  returns_ 1 pr "$p" 2>err </dev/null || fail=1
  printf '%s\n' "pr: $p: $ENOENT" >exp || framework_failure_
  compare exp err || fail=1
done

# number parsing issue
returns_ 1 pr --pages=-0 2>err </dev/null || fail=1
printf '%s\n' "pr: invalid --pages argument '-0'" >exp
compare exp err || fail=1

# number validation issue
returns_ 1 pr --pages=0 2>err </dev/null || fail=1
printf '%s\n' "pr: invalid page range '0'" >exp
compare exp err || fail=1

INV='invalid number'

returns_ 1 pr -l0 2>err </dev/null || fail=1
printf '%s\n' "pr: '-l PAGE_LENGTH' $INV of lines: '0': $ERANGE" \
 >exp || framework_failure_
compare exp err || fail=1

for w in w W; do
  returns_ 1 pr -${w}0 2>err </dev/null || fail=1
  printf '%s\n' "pr: '-$w PAGE_WIDTH' $INV of characters: '0': $ERANGE" \
   >exp || framework_failure_
  compare exp err || fail=1
done

returns_ 1 pr -e=-1 2>err </dev/null || fail=1
printf '%s\n' "pr: '-e' extra characters or $INV in the argument: '-1'" \
              "Try 'pr --help' for more information." \
 >exp || framework_failure_
compare exp err || fail=1

Exit $fail
