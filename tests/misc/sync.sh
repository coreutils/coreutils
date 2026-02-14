#!/bin/sh
# Test various sync(1) operations

# Copyright (C) 2015-2026 Free Software Foundation, Inc.

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
print_ver_ sync
getlimits_

touch file || framework_failure_

# fdatasync+syncfs is nonsensical
returns_ 1 sync --data --file-system || fail=1

# fdatasync needs an operand
returns_ 1 sync -d || fail=1

# Test syncing of file (fsync) (little side effects)
sync file || fail=1

# Test syncing of write-only file - which failed since adding argument
# support to sync in coreutils-8.24.
chmod 0200 file || framework_failure_
sync file || fail=1

# Ensure multiple args are processed and diagnosed
returns_ 1 sync file nofile || fail=1

# Ensure all arguments a processed even if one results in an error
returns_ 1 sync nofile1 file nofile2 2>err || fail=1
cat <<\EOF > exp || framework_failure_
sync: error opening 'nofile1': No such file or directory
sync: error opening 'nofile2': No such file or directory
EOF
compare exp err || fail=1

# Ensure inaccessible dirs give an appropriate error
mkdir norw || framework_failure_
chmod 0 norw || framework_failure_
if ! test -r norw; then
  returns_ 1 sync norw 2>errt || fail=1
  # AIX gives "Is a directory"
  sed "s/$EISDIR/$EACCES/" <errt >err || framework_failure_
  printf "sync: error opening 'norw': $EACCES\n" >exp
  compare exp err || fail=1
fi

if test "$fail" != '1'; then
  # Ensure a fifo doesn't block
  mkfifo_or_skip_ fifo
  for opt in '' '-f' '-d'; do
    timeout=10
    if test "$opt" = '-f'; then
      test "$RUN_VERY_EXPENSIVE_TESTS" = yes || continue
      timeout=60
    fi
    returns_ 124 timeout "$timeout" sync $opt fifo && fail=1
  done
fi

Exit $fail
