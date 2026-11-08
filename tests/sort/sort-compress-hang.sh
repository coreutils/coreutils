#!/bin/sh
# Test for sort --compress hang

# Copyright (C) 2010-2026 Free Software Foundation, Inc.

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
very_expensive_

cat <<EOF >compress || framework_failure_
#!$SHELL
tr 41 14 || exit
touch ok
EOF

chmod +x compress

seq -w 200000 > exp || fail=1
tac exp > in || fail=1

# When the bug occurs, 'sort' hangs forever.  When it doesn't occur,
# 'sort' could be running slowly on an overburdened machine.
# On a circa-2010 Linux server using NFS, a successful test completes
# in about 170 seconds, so specify 1700 seconds as a safety margin.
# Note --foreground will not kill any of the "compress" sub processes,
# assuming they're well behaved and exit in a timely manner, but will
# allow this command to be responsive to Ctrl-C
timeout --foreground 1700 sort --compress-program=./compress -S 1k in > out \
  || fail=1

compare exp out || fail=1
test -f ok || fail=1
rm -f ok || framework_failure_

# If $TMPDIR is relative, give subprocesses time to react when 'sort' exits.
# Otherwise, under NFS, when 'sort' unlinks the temp files and they
# are renamed to .nfsXXXX instead of being removed, the parent cleanup
# of this directory will fail because the files are still open.
case $TMPDIR in
/*) ;;
*) sleep 1;;
esac

# Make sure that 'sort' doesn't hang when the output file is given as
# the input multiple times.
# FIXME: This assumes the input file is larger than the default pipe size.
seq -w 100000 >exp1 &&
sed p exp1 >exp2 &&
cp exp1 inp || framework_failure_
timeout 60 sort -m --compress-program=./compress -o inp inp inp >out 2>err \
  || fail=1
compare exp2 inp || fail=1
compare /dev/null out || fail=1
compare /dev/null errr || fail=1
cp exp1 inp || framework_failure_
timeout 60 sort -m --compress-program=./compress -o inp - - <inp >out 2>err \
  || fail=1
compare exp2 inp || fail=1
compare /dev/null out || fail=1
compare /dev/null err || fail=1
rm -f compress ok || framework_failure_

Exit $fail
