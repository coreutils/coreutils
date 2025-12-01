#!/bin/sh
# Ensure that cut uses bounded memory when possible.

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
print_ver_ cut

vm=$(get_min_ulimit_v_ cut -b1 /dev/null) ||
  skip_ 'failed to determine memory limit'

# There is no way to implement '-s -f1' without allocating unbounded memory.
# Check the rest.
for opts in '-c1' '-b1' '-s -f2' '-f1'; do
  timeout 0.5 $SHELL -c \
    "(ulimit -v $(($vm+6000)) \
      && cut $opts </dev/zero >/dev/null 2>err)"
  ret=$?
  test -f err || skip_ 'shell ulimit failure'
  test $ret = 124 || {
    fail=1
    cat err
    echo "cut $opts used too much memory" >&2
  }
done

Exit $fail
