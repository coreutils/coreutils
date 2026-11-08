#!/bin/sh
# ensure that a mode string containing white space is rejected

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
print_ver_ chmod mkdir ginstall

touch f || framework_failure_
chmod 600 f || framework_failure_

for mode in ' 644' '644 ' ' u+x' 'u+x ' 'u+x, +755' 'u+x ,g+w' 'u+x,	g+w'; do
  returns_ 1 chmod "$mode" f 2>/dev/null || fail=1
  returns_ 1 mkdir -m "$mode" d 2>/dev/null || fail=1
  test -d d && fail=1
  returns_ 1 ginstall -m "$mode" f g 2>/dev/null || fail=1
  test -e g && fail=1
done

# The mode of f must not have changed.
test "$(stat --printf=%a f)" = 600 || fail=1

Exit $fail
