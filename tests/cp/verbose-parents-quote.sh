#!/bin/sh
# Ensure that 'cp --verbose --parents' quotes messages.

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
print_ver_ cp printf

NL='
'

mkdir -p a/b"$NL"c b &&
echo hello > a/b"$NL"c/f || framework_failure_

env printf "'a' -> 'b/a'\n"\
"%q -> %q\n"\
"%q -> %q\n" a/b"$NL"c b/a/b"$NL"c a/b"$NL"c/f b/a/b"$NL"c/f >exp-err \
  || framework_failure_

cp --parents --verbose a/b"$NL"c/f b >out >err || fail=1
compare /dev/null out || fail=1
compare exp-err err || fail=1

Exit $fail
