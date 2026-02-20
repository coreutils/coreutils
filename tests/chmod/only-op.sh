#!/bin/sh
# Test that 'chmod' does not skip calling chmod(2) even when the file mode bits
# are unchanged.

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
print_ver_ chmod stat
skip_if_root_

test "$(stat --format=%U /)" = 'root' || skip_ 'root does not own /'

for op in '+' '-' '='; do
  returns_ 1 chmod "$op" / 2>err || fail=1
  grep -F "chmod: changing permissions of '/'" err || { fail=1; cat err; }
done

Exit $fail
