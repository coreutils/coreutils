#!/bin/sh
# Test use of compression subprocesses by sort

# Copyright (C) 2010-2025 Free Software Foundation, Inc.

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
print_ver_ sort kill
expensive_

# Terminate any background processes
cleanup_() { kill $pid 2>/dev/null && wait $pid; }

SORT_FAILURE=2

seq -w 2000 > exp || framework_failure_
tac exp > in || framework_failure_
insize=$(stat -c %s - <in) || framework_failure_

# This compressor's behavior is adjustable via environment variables.
export PRE_COMPRESS=
export POST_COMPRESS=

printf '%s\n' '#!'"$SHELL" >compress || framework_failure_
cat <<\EOF >>compress || framework_failure_
eval "$PRE_COMPRESS"
tr 41 14 || exit
eval "$POST_COMPRESS"
EOF

chmod +x compress

# "Early exit" test
#
# In this test, the compressor exits before reading all (any) data.
# Until coreutils 9.9 'sort' could get a SIGPIPE writing to the
# exited processes and silently exit.  Note the same issue can happen
# irrespective of exit status.  It's more likely to happen in the
# case of the child exiting with success, and if we write more data
# (hence the --batch-size=30 and double "in").  Note we check sort doesn't
# get SIGPIPE rather than if it returns SORT_FAILURE, because there is
# the theoretical possibility that the kernel could buffer the
# amount of data we're writing here and not issue the EPIPE to sort.
# In other words we currently may not detect failures in the extreme edge case
# of writing a small amount of data to a compressor that exits 0
# while not reading all the data presented.
PRE_COMPRESS='exit 0' \
 sort --compress-program=./compress -S 1k --batch-size=30 ./in ./in > out
test $(env kill -l $?) = 'PIPE' && fail=1

# "Impatient exit" tests
#
# In these test cases, the biggest compressor (or decompressor) exits
# with nonzero status, after sleeping a bit.  Until coreutils 8.7
# 'sort' impatiently exited without waiting for its decompression
# subprocesses in these cases.  Check compression too, while we're here.
#
for compress_arg in '' '-d'
do
  POST_COMPRESS='
    test "X$1" != "X'$compress_arg'" || {
      test "X$1" = "X" && exec <&1
      size=$(stat -c %s -)
      exec >/dev/null 2>&1 <&1 || exit
      expr $size "<" '"$insize"' / 2 || { sleep 1; exit 1; }
    }
  ' sort --compress-program=./compress -S 1k --batch-size=2 in > out
  test $? -eq $SORT_FAILURE || fail=1
done

# "Pre-exec child" test
#
# Ignore a random child process created before 'sort' was exec'ed.
# This bug was also present in coreutils 8.7.
#
( (sleep 1; exec false) & pid=$!
  PRE_COMPRESS='test -f ok || sleep 2'
  POST_COMPRESS='touch ok'
  exec sort --compress-program=./compress -S 1k in >out
) || fail=1
compare exp out || fail=1
test -f ok || fail=1
rm -f ok

rm -f compress

# If $TMPDIR is relative, give subprocesses time to react when 'sort' exits.
# Otherwise, under NFS, when 'sort' unlinks the temp files and they
# are renamed to .nfsXXXX instead of being removed, the parent cleanup
# of this directory will fail because the files are still open.
case $TMPDIR in
/*) ;;
*) sleep 1;;
esac

Exit $fail
