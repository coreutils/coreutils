#!/bin/sh
# Test for proper detection of EPIPE with ignored SIGPIPE

# Copyright (C) 2016 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ seq

# upon EPIPE with signals ignored, 'seq' should exit with an error.
(trap '' PIPE;
 timeout 10 sh -c '( (seq inf 2>err ; echo $?>code) | head -n1)'>/dev/null)

# Exit-code must be 1, indicating 'write error'
cat << \EOF > exp || framework_failure_
1
EOF
if test -e code ; then
  compare exp code || fail=1
else
  # 'exitcode' file was not created
  warn_ "missing exit code file (seq failed to detect EPIPE?)"
  fail=1
fi

# The error message must begin with "standard output:"
# (but don't hard-code the strerror text)
compare_dev_null_ /dev/null err
if ! grep -qE '^seq: standard output: .+$' err ; then
  warn_ "seq emitted incorrect error on EPIPE"
  fail=1
fi

Exit $fail
