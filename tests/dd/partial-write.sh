#!/bin/sh
# Ensure partial writes are properly diagnosed

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
print_ver_ dd

(
  ulimit -S -f 1024 || skip_ 'unable to set file size ulimit'
  trap '' XFSZ || skip_ 'unable to ignore SIGXFSZ'
  dd if=/dev/zero of=f bs=768K count=2 2>err
)
ret=$?

if test $ret = 1; then
  test -s f || skip_ 'The system disallowed all writes'
  grep -F '+1 records out' err || { cat err; fail=1; }
elif test $ret = 0; then
  skip_ 'The system did not limit the file fize'
else
  framework_failure_
fi

Exit $fail
