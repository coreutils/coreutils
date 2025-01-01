#!/bin/sh
# Test mv --exchange.

# Copyright 2024-2025 Free Software Foundation, Inc.

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
print_ver_ mv


# Test exchanging files.
touch a || framework_failure_
mkdir b || framework_failure_
if ! mv -T --exchange a b 2>errt; then
  # AIX gives "Unsupported attribute value" (errno 124)
  # NetBSD and OpenBSD give "Not supported"
  sed 's/Not /not /; s/[Uu]nsupported/not supported/' < errt > exchange_err
  grep 'not supported' exchange_err || { cat exchange_err; fail=1; }
else
  test -d a || fail=1
  test -f b || fail=1
fi

# Test wrong number of arguments.
touch c || framework_failure_
returns_ 1 mv --exchange a 2>/dev/null || fail=1
returns_ 1 mv --exchange a b c 2>/dev/null || fail=1

# Both files must exist.
returns_ 1 mv --exchange a d 2>/dev/null || fail=1

Exit $fail
