# Ensure chmod --reference processes operands in command-line order

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
print_ver_ chmod


touch ref c a b || framework_failure_
chmod 644 ref && chmod 600 c a b || framework_failure_

chmod -v --reference=ref c a b > out || fail=1
cat <<\EOF > exp || framework_failure_
mode of 'c' changed from 0600 (rw-------) to 0644 (rw-r--r--)
mode of 'a' changed from 0600 (rw-------) to 0644 (rw-r--r--)
mode of 'b' changed from 0600 (rw-------) to 0644 (rw-r--r--)
EOF
compare exp out || fail=1

# The reference mode removes the search bit from dir, so dir/file
# must be handled before dir.
mkdir dir && touch dir/file || framework_failure_
chmod 600 dir/file || framework_failure_
chmod --reference=ref dir/file dir || fail=1
chmod 755 dir || framework_failure_
test "$(stat -c %a dir/file)" = 644 || fail=1

Exit $fail
