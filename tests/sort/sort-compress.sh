#!/bin/sh
# Test use of compression by sort

# Copyright (C) 2007-2026 Free Software Foundation, Inc.

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
print_ver_ sort
require_trap_signame_

seq -w 2000 > exp || framework_failure_
tac exp > in || framework_failure_

# This should force the use of temp files
sort -S 1k in > out || fail=1
compare exp out || fail=1

# Create our own gzip program that will be used as the default
cat <<EOF > gzip || framework_failure_
#!$SHELL
tr 41 14
touch ok
EOF

chmod +x gzip

# Ensure 'sort' is immune to parent's SIGCHLD handler
# Use a subshell and an exec to work around a bug in FreeBSD 5.0 /bin/sh.
(
  trap '' CHLD

  # This should force the use of child processes for "compression"
  PATH=.:$PATH exec sort -S 1k --compress-program=gzip in > /dev/null
) || fail=1

# This will find our new gzip in PATH
PATH=.:$PATH sort -S 1k --compress-program=gzip in > out || fail=1
compare exp out || fail=1
test -f ok || fail=1
rm -f ok

# Test that an unset PATH is not treated the same as an empty
# PATH, i.e., PATH=''.  On GNU/Linux, an unset PATH searches
# the default path, but an empty PATH searches the current working
# directory.  Since the behavior may differ on other platforms, we
# check it using 'env' first.  In coreutils-9.9 to coreutils-9.12,
# an unset PATH was turned into an empty PATH.
sort=$(cmd_exe sort) || framework_failure_
env -u PATH gzip </dev/null >/dev/null 2>&1
if test $? -eq 0 && ! test -f ok; then
  env -u PATH "$sort" -S 1k --compress-program=gzip in >out || fail=1
  compare exp out || fail=1
  test -f ok && fail=1
fi
rm -f ok || framework_failure_

# Also test the behavior with PATH set to an empty string.
tr=$(cmd_exe tr) &&
touch=$(cmd_exe touch) &&
cat <<EOF >abs-gzip &&
#!$SHELL
$tr 41 14
$touch ok
EOF
chmod +x abs-gzip || framework_failure_
env PATH= abs-gzip </dev/null >/dev/null 2>&1
if test $? -eq 0 && test -f ok; then
  rm -f ok || framework_failure_
  env PATH= "$sort" -S 1k --compress-program=abs-gzip in >out || fail=1
  compare exp out || fail=1
  test -f ok || fail=1
fi
rm -f ok || framework_failure_

# This is to make sure it works with no compression.
PATH=.:$PATH sort -S 1k in > out || fail=1
compare exp out || fail=1
test -f ok && fail=1

# This is to make sure we can use something other than gzip
mv gzip dzip || fail=1
sort --compress-program=./dzip -S 1k in > out || fail=1
compare exp out || fail=1
test -f ok || fail=1
rm -f ok

# Make sure it can find other programs in PATH correctly
PATH=.:$PATH sort --compress-program=dzip -S 1k in > out || fail=1
compare exp out || fail=1
test -f ok || fail=1
rm -f dzip ok

# Check the behavior of 'sort' when the program specified by --compress-program
# does not exist.
cat <<\EOF > exp-err
sort: could not run compress program 'missing': No such file or directory
EOF
sort --compress-program=missing -S 1k in > out 2> err || fail=1
compare exp out || fail=1
compare exp-err err || fail=1

Exit $fail
