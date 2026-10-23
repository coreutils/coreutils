#!/bin/sh
# Ensure exit with failure if only some operations succeed

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
getlimits_

touch file || framework_failure_
returns_ 1 chmod 0 missing_file file || fail=1

touch unreadable || framework_failure_
chmod 0 unreadable || framework_failure_
if ! test -r unreadable; then
  test -r file && fail=1
fi

# Ensure a name that is too long is diagnosed, and later operands
# are still processed, also with -R.
# We apply a limit since AIX returns 2^32-1 which would trigger resource issues.
name_max=$(stat -f -c %l .) && test "$name_max" -lt $((1024*1024)) ||
  name_max=
if test -n "$name_max"; then
  long_name=$(printf '%*s' $(($name_max + 1)) | tr ' ' '0')
  cat <<EOF > exp || framework_failure_
chmod: cannot access '$long_name': $ENAMETOOLONG
EOF
  for opt in '' -R; do
    chmod 600 file || framework_failure_
    returns_ 1 chmod $opt 700 "$long_name" file 2> err || fail=1
    compare exp err || fail=1
    test "$(stat -c %a file)" = 700 || fail=1
  done
  cat <<EOF > exp || framework_failure_
chmod: failed to get attributes of '$long_name': $ENAMETOOLONG
EOF
  returns_ 1 chmod --reference="$long_name" file 2> err || fail=1
  compare exp err || fail=1
fi

Exit $fail
